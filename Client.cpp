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

	for (;;) {
		if (exit_thread || _offset == _file_size)
			break;
		std::this_thread::sleep_for(1s);
	}

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

	check_sum(_dest_file, _file_size);
}

// @NOTE:
// adapted from boost docs for crc_example.cpp
bool Client::check_sum(std::fstream& file, size_t file_siz)
{
	if (_checksum == Shared::_CRC_HASH_NOT_FOUND)
		return true;

	boost::crc_32_type checker;

	do
	{
		char buffer[ BUFF_SIZE ];
		file.read(buffer, BUFF_SIZE);
		checker.process_bytes(buffer, BUFF_SIZE);
	} while (file);

	std::string temp = base64_decode(_checksum);
	std::cout << "base64.h methods:\n";
	std::cout << "base64_decode(_checksum): " << temp << "\n";

	std::cout << "_checksum in base64 untouced = " << _checksum << "\n";
	std::cout << "_checksum in hex (incorrect) = " << Shared::convert_base64_to_hex(_checksum) << "\n";
	std::cout << "boost::crc_32_type in hex = ";
	std::cout << std::hex << std::uppercase << checker.checksum() << std::endl;

	std::cout << "base64_encode(boost::checker):\n";
	std::stringstream ss;
	//@TODO: should it be changed to uppercase?
	ss << std::hex << std::uppercase << checker.checksum();
	//std::cout << base64_encode(ss.str()) << std::endl;
	std::string middle_rep (ss.str());
	char const* bytes_to_encode = middle_rep.data();
	unsigned int in_len = middle_rep.length();
	std::string final_rep = base64_encode((unsigned char const *)bytes_to_encode, in_len);

	std::cout << "with uppercase: " << final_rep << "\n";

	std::stringstream sss;
	sss << std::hex << checker.checksum();
	middle_rep = sss.str();
	bytes_to_encode = middle_rep.data();
	in_len = middle_rep.length();
	final_rep = base64_encode((unsigned char const *)bytes_to_encode, in_len);

	std::cout << "without uppercase: " << final_rep << std::endl;


	/*
	std::stringstream checker_stream;
	checker_stream << std::hex << std::uppercase << checker.checksum();
	std::string checker_hex_str;
	checker_hex_str = checker_stream.str();
	*/

	// @TODO: need to convert base64 to base16.
	// this will need a function since it involves padding, and conversions

	return true; //@TODO: replace with an actual logic check of equivalence
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
	boost::system::error_code ec;
	size_t len = socket.read_some(boost::asio::buffer(buff), ec);

	// this is for error handling, redo the above steps until a success occurs
	if (ec == boost::asio::error::eof) {
		//auto crlf_pos = Shared::find_crlfsuffix_in(buff);
		//std::string header (buff.begin(), crlf_pos);
		//std::cerr << "Http Response:\n"
		//std::cerr << header << std::endl;
		std::cerr << "EOF prematurely reached. Restarting worker_thread_run";
		std::cerr << std::endl;

		// @NOTE:
		// contemplate replacing this while loop,
		// with a recursive call to worker_thread_run() to eliminate
		// redundant code
		while (ec == boost::asio::error::eof) {
			socket.close();
			socket = Shared::connect_to_server(_host_url, _io_service);

			HttpRequest req (_host_url, _file_path);
			req.set_keepalive();
			req.set_range(br.first, br.last);
			req.simplify_accept();

			Shared::try_writing(socket, req.to_string());

			len = socket.read_some(boost::asio::buffer(buff), ec);
		}
	}
	else if (ec)
		throw boost::system::system_error(ec);

	auto crlf_pos = Shared::find_crlfsuffix_in(buff);

	std::string header (buff.begin(), crlf_pos);
	size_t body_len = len - header.length() - 4;
	auto body_pos = crlf_pos + 4;
	Shared::write_to_file(body_len, body_pos, buff.end(), outfile);

	size_t offset = body_len;

	while (!exit_thread) {
		if (offset == static_cast<size_t>(br.get_inclus_diff()))
			break;

		len = socket.read_some(boost::asio::buffer(buff), ec);

		// this is for error handling, if socket gets prematurely closed,
		// recursively repeat above steps
		if (ec == boost::asio::error::eof) {
			socket.close();
			std::remove(file_name.c_str());
			worker_thread_run(br, ID);
			return;
		}
		else if (ec)
			throw boost::system::system_error(ec);
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
	boost::system::error_code ec;
	size_t len = socket.read_some(boost::asio::buffer(buff), ec);
	auto crlf_pos = Shared::find_crlfsuffix_in(buff);
	if (crlf_pos == buff.end())
		throw "no crlf suffix";

	std::string header (buff.begin(), crlf_pos);
	size_t body_len = len - header.length() - 4; // 4 because CRLFCRLF
	auto body_pos = crlf_pos + 4; // 4 because CRLFCRLF
	Shared::write_to_file(body_len, body_pos, buff.end(), _dest_file);

	while (!exit_thread) {
		len = socket.read_some(boost::asio::buffer(buff), ec);
		if (ec == boost::asio::error::eof)
			break;
		else if (ec)
			throw boost::system::system_error(ec);
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

		auto temp = Shared::split(_file_path, "/");
		std::string file_name = temp.back();
		_dest_file.open(file_name, std::ios::out | std::ios::binary);

		std::vector<char> buff (BUFF_SIZE, '\0');
		boost::system::error_code ec;
		socket.read_some(boost::asio::buffer(buff), ec);

		auto crlf_pos = Shared::find_crlfsuffix_in(buff); // note: position of CRLFCRLF
		if (crlf_pos == buff.end())
			throw "crlf not in initial http response";

		std::string header (buff.begin(), crlf_pos);
		bool accepts_byte_ranges = Shared::check_accepts_byte_ranges(header);

		_file_size = Shared::parse_for_cont_length(header);
		_checksum  = Shared::parse_for_cdc32(header);

		std::cout << "blah: " << base64_decode(_checksum) << std::endl;
		return;

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
		poison_tasks();
		for (std::thread& t : _threads) {
			if (t.joinable())
				t.join();
		}
		throw;
	}
}

// @NOTE: functions below are not in use in this implementation
//
// were originally used in a 'pass-the-baton' implementation, where threads
// sent multiple chunk requests and in synch wrote to a destination file
// implementation was too slow, so took a different approach
//
// Kept as part of the file, for future implementations where this approach
// is modified to be efficient
//
bool Client::sync_file_write(ByteRange task, std::vector<char>::iterator start_pos,
		std::vector<char>::iterator end_pos)
{
	std::unique_lock<std::mutex> lock(_file_lock);

	std::atomic<int>& offset = _offset;
	_file_cv.wait_for(lock, 1s,
			[&task, &offset]{return task.offset_matches(offset);});

	if (!task.offset_matches(_offset))
		return false;

	for (int i = 0; i < task.get_inclus_diff(); i++) {
		if (start_pos == end_pos)
			throw std::string("start_pos == end_poos");
		_dest_file << *start_pos++;
	}
	_offset += task.get_inclus_diff();

	return true;
}

size_t Client::try_reading(std::vector<char>& main_buff, tcp::socket& socket, size_t payload_size)
{
	std::vector<char> temp_buff (BUFF_SIZE, '\0');
	boost::system::error_code ec;
	//main_buff.clear();

	size_t len = socket.read_some(boost::asio::buffer(temp_buff), ec);
	if (ec == boost::asio::error::eof) {
		auto crlf_pos = Shared::find_crlfsuffix_in(temp_buff);
		std::string header (temp_buff.begin(), crlf_pos);
		std::cerr << "Http Response:\n";
		std::cerr << header << std::endl;
		return _SIZE_MAX;
	}
	else if (ec)
		throw boost::system::system_error(ec);

	auto crlf_pos = Shared::find_crlfsuffix_in(temp_buff);

	int header_len = 0;
	for (auto it = temp_buff.begin(); it != crlf_pos; ++it)
		header_len++;

	// total length to read = header_len + 4 + payload_size
	// amount read thus far = len
	size_t amount_left_to_read = (header_len + 4 + payload_size) - len;

	// put data into main_buff (including header, for now)
	main_buff.clear();
	for (size_t i = 0; i < len; i++)
		main_buff.push_back(temp_buff[i]);

	len = boost::asio::read(socket, boost::asio::buffer(temp_buff),
			boost::asio::transfer_exactly(amount_left_to_read), ec);
	if (ec == boost::asio::error::eof) {
		std::string header (temp_buff.begin(), crlf_pos);
		std::cerr << "Http Response:\n";
		std::cerr << header << std::endl;
		return _SIZE_MAX;
	}
	else if (ec)
		throw boost::system::system_error(ec);

	for (size_t i = 0; i < len; i++)
		main_buff.push_back(temp_buff[i]);

	return main_buff.size();
}

bool Client::is_poison(const ByteRange task)
{
	if (task.first == POISON || task.last == POISON)
		return true;
	return false;
}

void Client::poison_tasks()
{
	size_t num_threads = _threads.size();
	ByteRange poison (POISON, POISON);
	_tasks.poison_self(poison, num_threads);
}
