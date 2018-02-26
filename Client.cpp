#include "Client.h"
#include <type_traits>

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


/*
void Client::write_to_file(std::ofstream& outfile, const std::pair<ByteRange, BufferPtr>& pair)
{
	int len = pair.first.get_inclus_diff();
	const BufferPtr& buffptr = pair.second;

	for (int i = 0; i < len; i++)
		outfile << buffptr->at(i);
}
*/

// @offset: variable that simultaneously keeps track of bytes read thus far,
// and the next byte (in array order) that the file writer is waiting on.
//
// @TODO: first http request asks for entire file. So socket probably gets messed up
// with that. Instead, maybe send a HEAD request?
void Client::parallel_download()
{
	//@TODO: honestly, these 4 lines can be put back in the calling function
	/*
	_offset = body_len;
	size_t i = 0;
	for (; buff_pos != buff.end() && i < body_len; ++buff_pos, ++i)
		outfile << *buff_pos;

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
	//
	//
	//now, simulate a worker thread
	tcp::socket& socket = *_sockptr; //@TODO: replace with _connect_to_server
	std::vector<char> sock_buff(BUFF_SIZE, '\0');
	for (;;) {
		std::experimental::optional<ByteRange> task = _tasks.get();
		if (task == std::experimental::nullopt || is_poison(*task))
			break;

		if (_file_size == _offset)
			break;

		if (exit_thread)
			break;

		HttpRequest req (_host_url, _file_path);
		req.set_keepalive();
		req.set_range(task->first, task->last);

		std::cout << "HttpRequest:\n";
		std::cout << req.to_string() << std::endl;

		try_writing_to_sock(socket, req.to_string());

		// zero out main buffer
		for (char& b : sock_buff)
			b = '\0';
		boost::system::error_code ec;
		size_t len = socket.read_some(boost::asio::buffer(sock_buff), ec);
		auto crlf_pos = find_crlfsuffix_in(sock_buff);

		int header_len = 0;
		for (auto it = sock_buff.begin(); it != crlf_pos && it != sock_buff.end(); ++it)
			header_len++;
		header_len += 4;

		//note: don't need file buff here
		len -= header_len;
		auto sb_it = crlf_pos + 4;
		std::cout << "error before writing" << std::endl;
		for (int i = 0; i < task->get_inclus_diff(); i++)
			outfile << *sb_it++;

		_offset += task->get_inclus_diff();
	}
	*/

	/*
	int offset = body_len; // bytes read thus far
	size_t i = 0;
	for (; buff_pos != buff.end() && i < body_len; ++buff_pos, ++i)
		outfile << *buff_pos;

	for (;;) {
		offset += CHUNK_SIZE;
		if (offset > _file_size) {
			int end = _file_size;
			ByteRange br (offset - CHUNK_SIZE, end - 1);
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

	offset = body_len; //reset offset to where it was.
	std::vector<std::pair<ByteRange, BufferPtr>> grabbed_results;

	while (!exit_thread) {

		if (offset == _file_size)
			break;

		auto pred = [offset] (std::pair<ByteRange, BufferPtr>& p)
		{ return p.first.offset_matches(offset); };

		auto it = std::find_if(grabbed_results.begin(),
				grabbed_results.end(), pred);

		if (it != grabbed_results.end()) {
			write_to_file(outfile, *it);
			offset += it->first.get_inclus_diff();
			grabbed_results.erase(it);
			continue;
		}

		auto result = _results.get();
		if (result == std::experimental::nullopt) {
			std::this_thread::sleep_for(1s);
			continue;
		}

		grabbed_results.push_back(std::move(*result));

	}

	if (exit_thread)
		poison_tasks();

	for (std::thread& t : _threads) {
		if (t.joinable())
			t.join();
	}
	*/
}

bool Client::is_poison(const ByteRange task)
{
	if (task.first == POISON || task.last == POISON)
		return true;
	return false;
}

void Client::worker_thread_run()
{
	/*
	tcp::socket socket = connect_to_server(_host_url);
	std::vector<char> sock_buff(BUFF_SIZE, '\0');

	for (;;) {
		std::experimental::optional<ByteRange> task = _tasks.get();
		if (task == std::experimental::nullopt || is_poison(*task))
			break;

		HttpRequest req(_host_url, _file_path);
		req.set_keepalive();
		req.set_range(task->first, task->last);

		try_writing_to_sock(socket, req.to_string());

		// zero out main buffer
		for (auto& b : sock_buff)
			b = '\0';
		boost::system::error_code ec;
		size_t len = socket.read_some(boost::asio::buffer(sock_buff), ec);
		auto crlf_pos = find_crlfsuffix_in(sock_buff); // 'CRLFCRLF'

		// get total size in bytes of header
		int header_len = 0;
		for (auto it = sock_buff.begin(); it != crlf_pos && it != sock_buff.end(); ++it)
			header_len++;
		header_len += 4; // 4 bytes for CRLFCRLF

		// @TODO: handle header and stuff. Check if more data needs to be read.
		// before transfering, apply integrity check to chunk.
		// Also check if http req needs to be resent.

		std::vector<char> file_buff(BUFF_SIZE, '\0'); // @TODO: consider changing to CHUNK_SIZE
		len -= header_len;
		auto fb_it = file_buff.begin();
		auto sb_it = crlf_pos + 4;
		for (size_t i = 0; i < len; i++)
			*fb_it++ = *sb_it++;

		BufferPtr buffptr = std::make_unique<std::vector<char>>(std::move(file_buff));
		//BufferPtr buffptr = std::make_shared<std::vector<char>>(file_buff);
		std::pair<ByteRange, BufferPtr> result = {*task, std::move(buffptr)};
		//_results.put(std::forward<std::pair<ByteRange, BufferPtr>>(result));
		//_results.put(std::move(result));
		_results.put(std::move(result));
		//@TODO: chhange to put_and_wait.
		//inside put_and_wait, call std::move() ?
	}

	//@TODO: send an HTTP Request with "close" instead of "keep-alive"?
	*/
}

void Client::poison_tasks()
{
	size_t num_threads = _threads.size();
	ByteRange poison (POISON, POISON);
	_tasks.poison_self(poison, num_threads);
}

void Client::simple_download()
{
	tcp::socket socket = connect_to_server(_host_url);

	HttpRequest req (_host_url, _file_path);
	try_writing_to_sock(socket, req.to_string());

	std::vector<char> buff (BUFF_SIZE, '\0');
	boost::system::error_code ec;
	size_t len = socket.read_some(boost::asio::buffer(buff), ec);
	auto crlf_pos = find_crlfsuffix_in(buff);
	if (crlf_pos == buff.end())
		throw "no crlf suffix";

	std::string header (buff.begin(), crlf_pos);
	size_t body_len = len - header.length() - 4;
	auto body_pos = crlf_pos + 4;
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

		//_sockptr = std::make_unique<tcp::socket>(connect_to_server(_host_url));
		//tcp::socket& socket = *_sockptr; //@TODO: this _sockptr is useful for the large part.
		tcp::socket socket = connect_to_server(_host_url);

		HttpRequest req (_host_url, _file_path);
		req.set_head();
		//req.set_keepalive();
		try_writing_to_sock(socket, req.to_string());

		// open file, read from socket and write to file
		//std::ofstream outfile;
		auto temp = split_(_file_path, "/");
		std::string file_name = temp.back();
		_dest_file.open(file_name, std::ios::out | std::ios::binary);

		std::vector<char> buff (BUFF_SIZE, '\0');
		boost::system::error_code ec;
		size_t len = socket.read_some(boost::asio::buffer(buff), ec);

		auto crlf_pos = find_crlfsuffix_in(buff); // note: position of CRLFCRLF
		if (crlf_pos == buff.end())
			throw "crlf not in initial http response";

		std::string header (buff.begin(), crlf_pos);
		bool accepts_byte_ranges = check_accepts_byte_ranges(header);
		//size_t body_len = len - header.length() - 4; // 4 bc CRLFCRLF is 4 characters long
		//auto body_pos = crlf_pos + 4; // 4 bc CRLFCRLF is 4 characters long

		_file_size = parse_for_cont_length(header);

		accepts_byte_ranges = false;
		if (!accepts_byte_ranges)
			simple_download();
		else
			parallel_download();

		std::cout << "finished reading" << std::endl;
		outfile.close();

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

