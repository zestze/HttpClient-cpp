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

void Client::write_to_file(std::ofstream& outfile, const std::pair<ByteRange, BufferPtr>& pair)
{
	int len = pair.first.get_inclus_diff();
	const BufferPtr& buffptr = pair.second;

	for (int i = 0; i < len; i++)
		outfile << buffptr->at(i);
}

// @offset: variable that simultaneously keeps track of bytes read thus far,
// and the next byte (in array order) that the file writer is waiting on.
void Client::parallel_download(std::ofstream& outfile, std::vector<char>& buff,
		size_t body_len, std::vector<char>::iterator buff_pos)
{
	int offset = body_len; // bytes read thus far
	size_t i = 0;
	for (; buff_pos != buff.end() && i < body_len; ++buff_pos, ++i)
		outfile << *buff_pos;

	// @TODO: logic needs to be verified, by hand if possible.
	//int last_offset = offset;
	// every put is left-inclusive, right-exclusive i.e. [, )
	// since a _file_size of 8 means the last byte is byte #7
	// populate tasks
	for (;;) {
		offset += CHUNK_SIZE;
		if (offset > _file_size) {
			int end = _file_size;
			// insert into queue
			// q.put( [offset - CHUNK, end) )
			ByteRange br (offset - CHUNK_SIZE, end - 1);
			_tasks.put(br);
			break;
		}
		else {
			// insert CHUNK_SIZE into queue
			// q.put( [offset - CHUNK, offset) )
			ByteRange br (offset - CHUNK_SIZE, offset - 1);
			_tasks.put(br);
		}
	}

	// @TODO: make threads and set them to work.
	for (int i = 0; i < NUM_THREADS; i++) {
		std::thread thr(&Client::worker_thread_run, this);
		_threads.push_back(std::move(thr));
	}

	offset = body_len; //reset offset to where it was.
	std::vector<std::pair<ByteRange, BufferPtr>> grabbed_results;

	while (!exit_thread) {
		// first, check if grabbed_results has an element with ByteRange.start
		// matching the current file offset.
		// if so, handle it (write to file), erase it, and then continue;
		//
		// if not, check if _results is empty, use conditional variable to wait
		// for it to have something. IMPORTANT, make sure that conditional wait
		// is timed, so to check if should exit_thread.
		// if not empty, grab element.
		// if element's ByteRange.start matches the offset, write it to
		// file. Actually, already doing this at top of loop.
		//

		//if (k

		auto pred = [offset] (std::pair<ByteRange, BufferPtr> p)
		{ return p.first.offset_matches(offset); };

		auto it = std::find_if(grabbed_results.begin(),
				grabbed_results.end(), pred);

		if (it != grabbed_results.end()) {
			write_to_file(outfile, *it);
			offset += it->first.get_inclus_diff();
			grabbed_results.erase(it);
			continue;
		}

		// else, need to check if can read more.
		// @TODO: left off here

	}


	// populate queue @DONE
	// make threads @DONE
	// wake threads
	// manage file offset and writing
	// clean up
}

void Client::worker_thread_run()
{
	/* can omit this-> when accessing class members and functions */
	//std::string file_name = _file_path;
	//std::string file_path = this->_file_path;

	// psuedocode:
	// make a new socket to the server
	// loop, and grab whatever is on queue. if it's empty, break loop, and exit
	// if it's not empty, handle by making an HTTPrEQUEST
	// then read from socket into the buffer. Once done, push to the queue with
	// the read stuff.
	// @TODO: put limits, on a special conc_queue put() call, that with a conditional
	// variable, makes threads sleep while queue is a certain size.
	// put_and_wait()
	// get_and_notify()
	// which are both, only intended to be used with the 'tasks' queue
	//

	tcp::socket socket = connect_to_server(_host_url);
	std::vector<char> sock_buff(BUFF_SIZE, '\0');

	for (;;) {
		std::experimental::optional<ByteRange> task = _tasks.get();
		if (task == std::experimental::nullopt)
			break;

		HttpRequest req(_host_url, _file_path);
		req.set_keepalive();
		req.set_range(task->first, task->last);

		try_writing_to_sock(socket, req.to_string());

		//@TODO: temp and buff need to be switched. Using buff and creating a unique
		//ptr from it repeatedly, will just be changing the same buffer that all the
		//pointers are pointing at. buff will be the one receiving the socket info,
		//and temp will be the one getting made_unique. But in that case,
		//need to give 'temp' a better more descriptive name.
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

		std::vector<char> file_buff(BUFF_SIZE, '\0');
		len -= header_len;
		auto fb_it = file_buff.begin();
		auto sb_it = crlf_pos + 4;
		for (size_t i = 0; i < len; i++)
			*fb_it++ = *sb_it++;

		//@TODO: handle http response here, check if message needs to be
		//resent, or anything else.
		//throw it all in a function, and a while loop.
		//
		//
		//@TODO: now having issues, how does the buffer survive past the end
		//of this for loop? Add it to some random list? or store it somewhere?
		//at some point, if it's not deleted, there will be a resource leak.

		BufferPtr buffptr = std::make_unique<std::vector<char>>(buff);
		_results.put({*task, std::move(buffptr)});
		//@TODO: chhange to put_and_wait.
		//inside put_and_wait, call std::move() ?
	}

	//@TODO: send an HTTP Request with "close" instead of "keep-alive"?
}

void Client::poison_tasks()
{
	size_t num_threads = _threads.size();
	ByteRange poison (POISON, POISON);
	_tasks.poison_self(poison, num_threads);
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

		//accepts_byte_ranges = false;
		if (!accepts_byte_ranges)
			simple_download(outfile, buff, body_len, body_pos);
		else
			parallel_download(outfile, buff, body_len, body_pos);

		if (exit_thread) {
			poison_tasks();
			for (std::thread& t : _threads)
				t.join();
		}
	}
	catch (...)
	{
		exit_thread = true;
		poison_tasks();
		for (std::thread& t : _threads)
			t.join();
		throw;
	}
	// std::ofstream close on destruction because of RAII
}

