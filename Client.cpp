#include "Client.h"

//@NOTE: for signal handler, to let threads now they should exit
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


// @offset: variable that simultaneously keeps track of bytes read thus far,
// and the next byte (in array order) that the file writer is waiting on.
//
void Client::parallel_download()
{
	int offset = _offset; // temporary
	for (;;) {
		offset += _CHUNK_SIZE;
		if (offset > _file_size) {
			ByteRange br (offset - _CHUNK_SIZE, _file_size - 1);
			_tasks.put(br);
			break;
		}
		else {
			ByteRange br (offset - _CHUNK_SIZE, offset - 1);
			_tasks.put(br);
		}
	}

	for (int i = 0; i < _NUM_THREADS; i++) {
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

		try_writing_to_sock(socket, req.to_string());

		size_t len = try_reading(sock_buff, socket, task->get_inclus_diff());
		int i = 0;
		while (len == _SIZE_MAX) {
			std::cerr << "Http Request #" << i++ << ":\n";
			std::cerr << req.to_string() << std::endl;
			socket = connect_to_server(_host_url, _io_service);
			try_writing_to_sock(socket, req.to_string());
			len = try_reading(sock_buff, socket, task->get_inclus_diff());
		}

		auto crlf_pos = find_crlfsuffix_in(sock_buff);

		int header_len = 0;
		for (auto it = sock_buff.begin(); it != crlf_pos && it != sock_buff.end(); ++it) {
			header_len++;
		}
		header_len += 4;

		len -= header_len;
		if (static_cast<int>(len) != task->get_inclus_diff())
			throw std::string("len != inclusive_diff()");

		bool success = sync_file_write(*task, crlf_pos + 4, sock_buff.end());
		while (!success && !exit_thread)
			success = sync_file_write(*task, crlf_pos + 4, sock_buff.end());

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

	size_t len = socket.read_some(boost::asio::buffer(temp_buff), ec);
	if (ec == boost::asio::error::eof) {
		auto crlf_pos = find_crlfsuffix_in(temp_buff);
		std::string header (temp_buff.begin(), crlf_pos);
		std::cerr << "Http Response:\n";
		std::cerr << header << std::endl;
		return _SIZE_MAX;
	}
	else if (ec)
		throw boost::system::system_error(ec);

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

void Client::write_to_file(size_t amount_to_write, std::vector<char>::iterator start_pos,
		std::vector<char>::iterator end_pos)
{
	for (size_t i = 0; i < amount_to_write; i++) {
		if (start_pos == end_pos)
			throw std::string("start_pos == end_pos");
		_dest_file << *start_pos++;
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
	//for (size_t i = 0; body_pos != buff.end() && i < body_len; ++body_pos, ++i)
		//_dest_file << *body_pos;
	write_to_file(body_len, body_pos, buff.end());

	while (!exit_thread) {
		len = socket.read_some(boost::asio::buffer(buff), ec);
		if (ec == boost::asio::error::eof)
			break;
		else if (ec)
			throw boost::system::system_error(ec);
		//write_to_file(buff, len);
		write_to_file(len, buff.begin(), buff.end());
		//for (size_t i = 0; i < len; i++)
			//outfile << buff[i];
	}
	//std::cout << "end of simple_download" << std::endl;
}

void Client::run(bool force_simple)
{
	try {
		std::cout << "Starting client...\n";
		std::cout << "Type CTRL+C to quit" << std::endl;

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

