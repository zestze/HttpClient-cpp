/*
 * @TODO: parallel connections? The issue with this though, is that it seems
 * the fstream will remain a bottleneck as the (I'm assuming a thread implementation)
 * threads compete to write to the file. Even if the threads do manage to concurrently
 * write to a different section of the file, the filewriter head will have to jump
 * back and forth and that might be more expensive on traditional disc hardware.
 * Using a lock to synchronize writes will be expensive and might not result in a
 * performance improvement, since parallel downloads will still occur, but there'll
 * still only be 1 thread writing to file.
 *
 * Can get past this, by requesting small chunks just ahead of each other, so that
 * threads will quickly swap batons, so to speak.
 *
 * Also, will these threads all be connected through the same port? I don't think so:
 * each will have to open its own socket instance through a different port.
 *
 * Maybe make chunk size near buffer size?
 *
 * @TODO: integrity checksum
 * x-goog-hash, md5 and crc32c
 *
 * @TODO: Handle errors and retries
 *
 * @TODO: Benchmarks for various chunk size for parallel downloads
 *
 * @TODO: Limiting the number of concurrent chunks being downloaded to some max value
 *
 * @TODO: Resuming partially downloaded chunks on error
 *
 * @TODO: Calculating checksum for integrity check during download rather than at end
 *
 * @TODO: determine best buffer size?
 *
 * @TODO: implement a DASH?
 *
 * @TODO: instead of using a shared_ptr, use unique_ptr with move semantics
 *
 */

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <boost/asio.hpp>
#include <string>
#include <deque>
#include <array>
#include <algorithm>
#include <thread>
#include <experimental/optional>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <cstdio>

#include "ConcQueue.h"
#include "HttpRequest.h"
#include "shared.h"
#include "ByteRange.h"

#ifndef BUFF_SIZE
#define BUFF_SIZE 4096
#endif

#ifndef CHUNK_SIZE
#define CHUNK_SIZE (BUFF_SIZE / 2)
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 10
#endif

// already defined in shared.h
#ifndef POISON
#define POISON -2
#endif

// for reference 'seconds' as a 's' literal
using namespace std::chrono_literals;

// for boost asio socket functions
using boost::asio::ip::tcp;

//@NOTE: used to signify that a reread needs to occur.
const size_t _SIZE_MAX = std::numeric_limits<size_t>::max();

void signal_handler(int signal);
void set_globals();

class Client {
	public:
		Client(std::string url, std::string file, int c, int n)
			: _host_url{url}, _file_path{file}, _offset{0},
			_CHUNK_SIZE{c}, _NUM_THREADS{n}
		{
			set_globals(); // exit_thread = false;
			std::signal(SIGINT, signal_handler);
		}

		// @NOTE: these are intentionally deleted, since each
		// Client has private members without copy semantics
		Client() = delete;
		Client(const Client&) = delete;
		Client(Client&&) = delete;
		Client& operator= (Client) = delete;
		Client& operator= (const Client&) = delete;
		Client& operator= (Client&&) = delete;

		~Client() = default;

		bool is_poison(const ByteRange task)
		{
			if (task.first == POISON || task.last == POISON)
				return true;
			return false;
		}

		void poison_tasks()
		{
			size_t num_threads = _threads.size();
			ByteRange poison (POISON, POISON);
			_tasks.poison_self(poison, num_threads);
		}

		void write_to_file(size_t amount_to_write,
				std::vector<char>::iterator start_pos,
				std::vector<char>::iterator end_pos,
				std::ofstream& outfile);

		bool sync_file_write(ByteRange task,
				std::vector<char>::iterator start_pos,
				std::vector<char>::iterator end_pos);

		size_t try_reading(std::vector<char>& main_buff, tcp::socket& socket,
				size_t payload_size);

		void worker_thread_run(ByteRange br, int ID);

		void parallel_download();

		void simple_download();

		void run(bool force_simple);

	private:
		boost::asio::io_service _io_service;

		std::string _host_url;
		std::string _file_path;
		int _file_size;
		std::atomic<int> _offset;

		std::condition_variable _file_cv;
		std::ofstream _dest_file;
		std::mutex    _file_lock;

		std::vector<std::thread> _threads;
		ConcQueue<ByteRange> _tasks;

		const int _CHUNK_SIZE;
		const int _NUM_THREADS;
};

#endif
