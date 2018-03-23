#include "Client.h"

// @NOTE:
// for signal handler, to let threads now they should exit
//
std::atomic<bool> exit_thread;

void signal_handler(int signal)
{
	if (signal)
		exit_thread = true;
}

void set_globals()
{
	exit_thread = false;
}


// @NOTE:
// offset is a variable that simultaneously keeps track of bytes read thus far,
// and the next byte (in array order) that the file writer is waiting on.
//
void Client::parallel_download()
{
	const int chunk = _file_size / _NUM_THREADS + 1; // round up
	int temp_offset = _offset;
	for (int i = 1; i <= _NUM_THREADS; i++) {
		if (i == _NUM_THREADS) {
			ByteRange br (temp_offset, _file_size - 1);
			std::thread thr(&Client::worker_thread_run, this,
					br, i);
			_threads.push_back(std::move(thr));
		}
		else {
			ByteRange br (temp_offset, temp_offset + chunk - 1);
			std::thread thr(&Client::worker_thread_run, this,
					br, i);
			_threads.push_back(std::move(thr));
		}
		temp_offset += chunk;
	}

	while (!exit_thread || _offset != _file_size)
		std::this_thread::sleep_for(1s);

	for (std::thread& t : _threads) {
		if (t.joinable())
			t.join();
	}

	if (exit_thread) {
		for (int i = 1; i <= _NUM_THREADS; i++) {
			std::string file_name = "temp" + std::to_string(i);
			std::remove(file_name.c_str());
		}
		return;
	}

	// concenate all the temp files into the destination
	for (int i = 1; i <= _NUM_THREADS; i++) {
		std::string file_name = "temp" + std::to_string(i);
		std::ifstream infile (file_name, std::ios::in | std::ios::binary);
		_dest_file << infile.rdbuf();
		infile.close();
		std::remove(file_name.c_str());
	}

	bool has_integrity = check_sum();
	if (!has_integrity) {
		std::cerr << "WARNING: generated md5hash did not match";
		std::cerr << " md5hash value passed in http Header\n";
		std::cerr << "Please try again.\n";
	}
}

bool Client::check_sum()
{
	if (_checksum == Shared::_HASH_NOT_FOUND)
		return true;

	int pipe_fd[2];
	pipe(pipe_fd);
	pid_t pid = fork();

	std::string md5sum_result;

	if (pid == 0) {
		close(pipe_fd[0]);
		dup2(pipe_fd[1], 1);
		close(pipe_fd[1]);

		execl("/usr/bin/md5sum", "md5sum", _dest_file_path.c_str(),
				(char *)0);
	}
	else {
		wait(nullptr);
		char buffer [BUFF_SIZE] = {'\0'};
		close(pipe_fd[1]);
		read(pipe_fd[0], buffer, BUFF_SIZE);
		md5sum_result = buffer;
	}

	/*
	std::string md5_hash_openssl = Shared::inefficient_md5_hash(_dest_file_path);
	std::transform(md5_hash_openssl.begin(), md5_hash_openssl.end(),
			md5_hash_openssl.begin(), ::toupper);
	std::cout << "md5sum calculated from openssl: "
		<< md5_hash_openssl << std::endl;
		*/

	std::vector<std::string> temp = Shared::split(md5sum_result, " ");
	std::string md5hash = temp.front();
	std::transform(md5hash.begin(), md5hash.end(), md5hash.begin(), ::toupper);
	std::cout << "md5sum calculated hash:     " << md5hash << std::endl;

	std::string grabbed_hash = Shared::convert_base64_to_hex(_checksum);
	std::cout << "hash grabbed from http:     " << grabbed_hash << std::endl;

	return md5hash == grabbed_hash;
}

// @NOTE:
// error handling can be improved, by - when arriving on eof error -
// checking how much has been read, and send out a modified HTTP Request
// for remaining data to read, rather than completely restarting
// process.
void Client::worker_thread_run(ByteRange br, int ID)
{
	tcp::socket socket = Shared::connect_to_server(_host_url, _io_service);
	std::string file_name = "temp" + std::to_string(ID);
	std::ofstream outfile (file_name, std::ios::out | std::ios::binary);

	HttpRequest req (_host_url, _file_path);
	req.set_keepalive();
	req.set_range(br.first, br.last);
	req.simplify_accept();

	Shared::try_writing(socket, req.to_string());

	std::vector<char> buff (BUFF_SIZE, '\0');
	asio::error_code ec;
	size_t len = socket.read_some(asio::buffer(buff), ec);

	// this is for error handling, redo the above steps until a success occurs
	if (ec == asio::error::eof) {
		std::cerr << "EOF prematurely reached. Restarting worker_thread_run";
		std::cerr << std::endl;

		// @NOTE:
		// contemplate replacing this while loop,
		// with a recursive call to worker_thread_run() to eliminate
		// redundant code
		while (ec == asio::error::eof) {
			socket.close();
			socket = Shared::connect_to_server(_host_url, _io_service);

			HttpRequest req (_host_url, _file_path);
			req.set_keepalive();
			req.set_range(br.first, br.last);
			req.simplify_accept();

			Shared::try_writing(socket, req.to_string());

			len = socket.read_some(asio::buffer(buff), ec);
		}
	}
	else if (ec)
		throw asio::system_error(ec);

	auto crlf_pos = Shared::find_crlfsuffix_in(buff);

	std::string header (buff.begin(), crlf_pos);
	size_t body_len = len - header.length() - 4;
	auto body_pos = crlf_pos + 4;
	Shared::write_to_file(body_len, body_pos, buff.end(), outfile);

	size_t offset = body_len;

	while (!exit_thread) {
		if (offset == static_cast<size_t>(br.get_inclus_diff()))
			break;

		len = socket.read_some(asio::buffer(buff), ec);

		// this is for error handling, if socket gets prematurely closed,
		// recursively repeat above steps
		if (ec == asio::error::eof) {
			socket.close();
			std::remove(file_name.c_str());
			worker_thread_run(br, ID);
			return;
		}
		else if (ec)
			throw asio::system_error(ec);
		Shared::write_to_file(len, buff.begin(), buff.end(), outfile);
		offset += len;
	}

	outfile.close(); //should be implicitly called through destructor
	_offset += br.get_inclus_diff();
}

void Client::simple_download()
{
	tcp::socket socket = Shared::connect_to_server(_host_url, _io_service);

	HttpRequest req (_host_url, _file_path);
	Shared::try_writing(socket, req.to_string());

	std::vector<char> buff (BUFF_SIZE, '\0');
	asio::error_code ec;
	size_t len = socket.read_some(asio::buffer(buff), ec);
	auto crlf_pos = Shared::find_crlfsuffix_in(buff);
	if (crlf_pos == buff.end())
		throw "no crlf suffix";

	std::string header (buff.begin(), crlf_pos);
	size_t body_len = len - header.length() - 4; // 4 because CRLFCRLF
	auto body_pos = crlf_pos + 4; // 4 because CRLFCRLF
	Shared::write_to_file(body_len, body_pos, buff.end(), _dest_file);

	while (!exit_thread) {
		len = socket.read_some(asio::buffer(buff), ec);
		if (ec == asio::error::eof)
			break;
		else if (ec)
			throw asio::system_error(ec);
		Shared::write_to_file(len, buff.begin(), buff.end(), _dest_file);
	}
}

void Client::run(bool force_simple)
{
	try {
		std::cout << "Starting client...\n";
		std::cout << "Type CTRL+C to quit" << std::endl;

		tcp::socket socket = Shared::connect_to_server(_host_url, _io_service);

		HttpRequest req (_host_url, _file_path);
		req.set_head();
		//req.set_keepalive();
		Shared::try_writing(socket, req.to_string());

		_dest_file.open(_dest_file_path, std::ios::out | std::ios::binary);

		std::vector<char> buff (BUFF_SIZE, '\0');
		asio::error_code ec;
		socket.read_some(asio::buffer(buff), ec);

		auto crlf_pos = Shared::find_crlfsuffix_in(buff); // note: position of CRLFCRLF
		if (crlf_pos == buff.end())
			throw "crlf not in initial http response";

		std::string header (buff.begin(), crlf_pos);
		bool accepts_byte_ranges = Shared::check_accepts_byte_ranges(header);

		_file_size = Shared::parse_for_cont_length(header);
		//_checksum  = Shared::parse_for_cdc32(header);
		_checksum = Shared::parse_for_md5(header);

		if (!accepts_byte_ranges || force_simple)
			simple_download();
		else
			parallel_download();

		_dest_file.close();
	}
	catch (...)
	{
		std::cerr << "caught error in Client::run()" << std::endl;
		exit_thread = true;
		//poison_tasks();
		for (std::thread& t : _threads) {
			if (t.joinable())
				t.join();
		}
		throw;
	}
}
