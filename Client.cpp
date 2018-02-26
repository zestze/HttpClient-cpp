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

	for (int i = 0; i < NUM_THREADS; i++) {
		std::thread thr(&Client::worker_thread_run, this);
		_threads.push_back(std::move(thr));
	}

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
	//std::vector<char> sock_buff(BUFF_SIZE, '\0');
	std::vector<char> sock_buff; //@TODO: change back to pre-allocate
	for (;;) {
		if (_offset == _file_size || exit_thread)
			break;
		std::experimental::optional<ByteRange> task = _tasks.get();
		if (task == std::experimental::nullopt || is_poison(*task))
			break;

		HttpRequest req (_host_url, _file_path);
		req.set_keepalive();
		req.set_range(task->first, task->last);
		req.simplify_accept();

		//std::cout << "HttpRequest:\n";
		//std::cout << req.to_string() << std::endl;

		try_writing_to_sock(socket, req.to_string());

		/*
		boost::system::error_code ec;
		size_t len = socket.read_some(boost::asio::buffer(sock_buff), ec);
		*/
		size_t len = try_reading(sock_buff, socket, task->get_inclus_diff());

		auto crlf_pos = find_crlfsuffix_in(sock_buff);

		int header_len = 0;
		//std::cout << "Header response:\n";
		for (auto it = sock_buff.begin(); it != crlf_pos && it != sock_buff.end(); ++it) {
			//std::cout << *it;
			header_len++;
		}
		//std::cout << std::endl;
		header_len += 4;

		len -= header_len;
		if (static_cast<int>(len) != task->get_inclus_diff())
			throw std::string("len != inclusive_diff()");

		bool success = sync_write(*task, crlf_pos + 4, sock_buff.end());
		while (!success && !exit_thread)
			success = sync_write(*task, crlf_pos + 4, sock_buff.end());

		/*
		auto sb_it = crlf_pos + 4;
		for (int i = 0; i < task->get_inclus_diff(); i++)
			_dest_file << *sb_it++;

		_offset += task->get_inclus_diff();
		*/
		//@TODO: doesn't synchronize access to file writer.
		//Need to figure that out.
	}
}

// @NOTE: instead, do first read outside of loop. Already know how many bytes of data
// going to read, so count how many bytes of header there are, add that (including crlfcrlf)
// to total bytes_to_read, and read that many.
size_t Client::try_reading(std::vector<char>& main_buff, tcp::socket& socket, size_t payload_size)
{
	std::vector<char> temp_buff (BUFF_SIZE, '\0');
	boost::system::error_code ec;
	//main_buff.clear(); //@TODO: get rid of this and push_back, manipulate by position instead

	size_t len = socket.read_some(boost::asio::buffer(temp_buff));

	auto crlf_pos = find_crlfsuffix_in(temp_buff);

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
			boost::asio::transfer_exactly(amount_left_to_read));

	for (size_t i = 0; i < len; i++)
		main_buff.push_back(temp_buff[i]);

	/*
	for (;;) {
		len = socket.read_some(boost::asio::buffer(temp_buff), ec);
		if (ec == boost::asio::error::eof)
			break;
		else if (ec)
			throw boost::system::system_error(ec);
		for (size_t i = 0; i < len; i++)
			main_buff.push_back(temp_buff[i]);
	}
	*/

	return main_buff.size();
}

bool Client::sync_write(ByteRange task, std::vector<char>::iterator start_pos,
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
	//std::cout << "end of simple_download" << std::endl;
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

		//accepts_byte_ranges = false;
		if (!accepts_byte_ranges)
			simple_download();
		else
			parallel_download();

		//std::cout << "finished reading" << std::endl;
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

