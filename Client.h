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
#define NUM_THREADS 1
#endif

// already defined in shared.h
#ifndef POISON
#define POISON -2
#endif

// for reference 'seconds' as a 's' literal
using namespace std::chrono_literals;

using boost::asio::ip::tcp;

void signal_handler(int signal);

class Client {
	public:
		Client(std::string url, std::string file)
			: _host_url{url}, _file_path{file} { }

		void parallel_download();

		void simple_download();

		void run();

		bool is_poison(const ByteRange task);

		void poison_tasks();

		void worker_thread_run();

		void write_to_file(std::vector<char>& buff, size_t len)
		{
			std::lock_guard<std::mutex> lock(_file_lock);
			for (size_t i = 0; i < len; i++)
				_dest_file << buff[i];
		}

	private:
		std::string _host_url;
		std::string _file_path;
		int _file_size;
		int _offset;

		std::ofstream _dest_file;
		std::mutex    _file_lock;

		std::vector<std::thread> _threads;
		ConcQueue<ByteRange> _tasks;
};

#endif
