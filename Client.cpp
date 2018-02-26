#include "Client.h"

// ************** GLOBALS *************
std::atomic<bool> exit_thread;
// ************** GLOBALS *************

void signal_handler(int signal)
{
	if (signal)
		exit_thread = true;
}

void set_globals()
{
	exit_thread = false;
}


// @offset: variable that simultaneously keeps track of bytes read thus far,
// and the next byte (in array order) that the file writer is waiting on.
//
void Client::parallel_download()
{
	int offset = _offset; // temporary
	for (;;) {
		offset += CHUNK_SIZE;
		if (offset > _file_size) {
			ByteRange br (offset - CHUNK_SIZE, _file_size - 1);
			_tasks.put(br);
			break;
		}
		else {
			ByteRange br (offset - CHUNK_SIZE, offset - 1);
			_tasks.put(br);
		}
	}

	//for (int i = 0; i < NUM_THREADS; i++) {
		//std::thread thr(&Client::worker_thread_run, this);
		//_threads.push_back(std::move(thr));
	//}

	for (;;) {
		if (exit_thread || _offset == _file_size)
			break;
		std::this_thread::sleep_for(1s);
	}

	if (exit_thread)
		poison_tasks();

	for (std::thread& t : _threads) {
		if (t.joinable())
			t.join();
	}
}

void Client::worker_thread_run()
{
	tcp::socket socket = connect_to_server(_host_url, _io_service);
	std::vector<char> sock_buff(BUFF_SIZE, '\0');
	for (;;) {
		std::experimental::optional<ByteRange> task = _tasks.get();
		if (task == std::experimental::nullopt || is_poison(*task))
			break;
		if (_offset == _file_size || exit_thread)
			break;

		HttpRequest req (_host_url, _file_path);
		req.set_keepalive();
		req.set_range(task->first, task->last);

		std::cout << "HttpRequest:\n";
		std::cout << req.to_string() << std::endl;

		try_writing_to_sock(socket, req.to_string());

		boost::system::error_code ec;
		size_t len = socket.read_some(boost::asio::buffer(sock_buff), ec);
		auto crlf_pos = find_crlfsuffix_in(sock_buff);

		int header_len = 0;
		for (auto it = sock_buff.begin(); it != crlf_pos && it != sock_buff.end(); ++it)
			header_len++;
		header_len += 4;

		len -= header_len;
		auto sb_it = crlf_pos + 4;
		for (int i = 0; i < task->get_inclus_diff(); i++)
			_dest_file << *sb_it++;

		_offset += task->get_inclus_diff();
		//@TODO: doesn't synchronize access to file writer.
		//Need to figure that out.
	}
}

void Client::simple_download()
{
	tcp::socket socket = connect_to_server(_host_url, _io_service);

	HttpRequest req (_host_url, _file_path);
	try_writing_to_sock(socket, req.to_string());

	std::vector<char> buff (BUFF_SIZE, '\0');
	boost::system::error_code ec;
	size_t len = socket.read_some(boost::asio::buffer(buff), ec);
	auto crlf_pos = find_crlfsuffix_in(buff);
	if (crlf_pos == buff.end())
		throw "no crlf suffix";

	std::string header (buff.begin(), crlf_pos);
	size_t body_len = len - header.length() - 4; // 4 because CRLFCRLF
	auto body_pos = crlf_pos + 4; // 4 because CRLFCRLF
	for (size_t i = 0; body_pos != buff.end() && i < body_len; ++body_pos, ++i)
		_dest_file << *body_pos;

	while (!exit_thread) {
		len = socket.read_some(boost::asio::buffer(buff), ec);
		if (ec == boost::asio::error::eof)
			break;
		else if (ec)
			throw boost::system::system_error(ec);
		write_to_file(buff, len);
		//for (size_t i = 0; i < len; i++)
			//outfile << buff[i];
	}
	std::cout << "end of simple_download" << std::endl;
}

void Client::run()
{
	try {
		std::cout << "Starting client...\n";
		std::cout << "Type CTRL+C to quit" << std::endl;

		set_globals();
		std::signal(SIGINT, signal_handler);

		tcp::socket socket = connect_to_server(_host_url, _io_service);

		HttpRequest req (_host_url, _file_path);
		req.set_head();
		//req.set_keepalive();
		try_writing_to_sock(socket, req.to_string());

		auto temp = split_(_file_path, "/");
		std::string file_name = temp.back();
		_dest_file.open(file_name, std::ios::out | std::ios::binary);

		std::vector<char> buff (BUFF_SIZE, '\0');
		boost::system::error_code ec;
		socket.read_some(boost::asio::buffer(buff), ec);

		auto crlf_pos = find_crlfsuffix_in(buff); // note: position of CRLFCRLF
		if (crlf_pos == buff.end())
			throw "crlf not in initial http response";

		std::string header (buff.begin(), crlf_pos);
		bool accepts_byte_ranges = check_accepts_byte_ranges(header);

		_file_size = parse_for_cont_length(header);

		accepts_byte_ranges = false;
		if (!accepts_byte_ranges)
			simple_download();
		else
			parallel_download();

		std::cout << "finished reading" << std::endl;
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
	// std::ofstream close on destruction because of RAII
}

