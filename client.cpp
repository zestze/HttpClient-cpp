#include "client.h"

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

void Client::simple_download(std::ofstream& outfile, std::vector<char>& buff,
		size_t len, std::vector<char>::iterator buff_pos)
{
	tcp::socket& socket = *_sockptr;
	size_t i = 0;
	for (; buff_pos != buff.end() && i < len; ++buff_pos, ++i)
		outfile << *buff_pos;

	// lazy solution @TODO: grab content-length and parse that instead of
	// reading until none left
	//int content_length = parse_for_cont_length(header);
	boost::system::error_code ec;
	while (!exit_thread) {
		for (char& c : buff)
			c = '\0';
		len = socket.read_some(boost::asio::buffer(buff), ec);
		if (ec == boost::asio::error::eof)
			break;
		else if (ec)
			throw boost::system::system_error(ec);
		for (size_t i = 0; i < len; i++)
			outfile << buff[i];
	}
}

void Client::parallel_download(std::ofstream& outfile, std::vector<char>& buff,
		size_t len, std::vector<char>::iterator buff_pos)
{
	int offset = len; // bytes read thus far
	size_t i = 0;
	for (; buff_pos != buff.end() && i < len; ++buff_pos, ++i)
		outfile << *buff_pos;

	// @TODO: logic needs to be verified, by hand if possible.
	//int last_offset = offset;
	// every put is left-inclusive, right-exclusive i.e. [, )
	// since a _file_size of 8 means the last byte is byte #7
	for (;;) {
		offset += CHUNK_SIZE;
		if (offset > _file_size) {
			int end = _file_size;
			// insert into queue
			// q.put( [offset - CHUNK, end) )
			Byte_Range br (offset - CHUNK_SIZE, end - 1);
			break;
		}
		else {
			// insert CHUNK_SIZE into queue
			// q.put( [offset - CHUNK, offset) )
		}
	}

	// populate queue
	// make threads
	// wake threads
	// manage file offset and writing
	// clean up
}

void Client::run()
{
	try {
		std::cout << "Starting client...\n";
		std::cout << "Type CTRL+C to quit" << std::endl;

		set_globals();
		std::signal(SIGINT, signal_handler);

		_sockptr = std::make_unique<tcp::socket>(connect_to_server(_host_url));
		tcp::socket& socket = *_sockptr;

		HttpRequest req (_host_url, _file_path);
		try_writing_to_sock(socket, req.to_string());

		// open file, read from socket and write to file
		std::ofstream outfile;
		auto temp = split_(_file_path, "/");
		std::string file_name = temp.back();
		outfile.open(file_name, std::ios::out | std::ios::binary);

		std::vector<char> buff(BUFF_SIZE, '\0');
		boost::system::error_code ec;
		size_t len = socket.read_some(boost::asio::buffer(buff), ec);

		auto crlf_pos = find_crlfsuffix_in(buff); // note: position of CRLFCRLF
		if (crlf_pos == buff.end())
			throw "crlf not in initial http response";

		std::string header (buff.begin(), crlf_pos);
		bool accepts_byte_ranges = check_accepts_byte_ranges(header);
		size_t body_len = len - header.length() - 4; // 4 bc CRLFCRLF is 4 characters long
		auto body_pos = crlf_pos + 4; // 4 bc CRLFCRLF is 4 characters long

		_file_size = parse_for_cont_length(header);

		if (!accepts_byte_ranges)
			simple_download(outfile, buff, body_len, body_pos);
		else
			parallel_download(outfile, buff, body_len, body_pos);

		if (exit_thread) {
			// don't forget to poison threads.
			for (std::thread& t : _threads)
				t.join();
			_threads.clear();
		}

	}
	catch (...)
	{
		exit_thread = true;
		for (std::thread& t : _threads)
			t.join();
		_threads.clear();
		throw;
	}
	// std::ofstream close on destruction because of RAII
}

