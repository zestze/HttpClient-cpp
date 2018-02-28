/*
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
 */

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <thread>
#include <experimental/optional>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <cstdio>
#include <boost/asio.hpp>
#include <boost/crc.hpp>
#include <typeinfo>

#include "ConcQueue.h"
#include "HttpRequest.h"
#include "shared.h"
#include "ByteRange.h"

#ifndef BUFF_SIZE
#define BUFF_SIZE 4096
#endif

#ifndef POISON
#define POISON -2
#endif

// for referencing 'seconds' as a 's' literal
using namespace std::chrono_literals;

// for boost asio socket functions
using boost::asio::ip::tcp;

// @NOTE:
// not in use in this implementation
// used to signify that a reread needs to occur.
//
const size_t _SIZE_MAX = std::numeric_limits<size_t>::max();

void signal_handler(int signal);
void set_globals();

class Client {
	public:
		Client(std::string url, std::string file, int n)
			: _host_url{url}, _file_path{file}, _offset{0},
			_NUM_THREADS{n}
		{
			set_globals();
			std::signal(SIGINT, signal_handler);
		}

		// @NOTE:
		// these are intentionally deleted, since each
		// Client has private members without copy semantics
		//
		Client() = delete;
		Client(const Client&) = delete;
		Client(Client&&) = delete;
		Client& operator= (Client) = delete;
		Client& operator= (const Client&) = delete;
		Client& operator= (Client&&) = delete;

		~Client() = default;

		void worker_thread_run(ByteRange br, int ID);

		void parallel_download();

		void simple_download();

		void run(bool force_simple);

		bool check_sum(std::fstream& file, size_t file_size);

		// @NOTE: not in use in this implementation
		bool is_poison(const ByteRange task);

		// @NOTE: not in use in this implementation
		void poison_tasks();

		// @NOTE: not in use in this implementation
		bool sync_file_write(ByteRange task,
				std::vector<char>::iterator start_pos,
				std::vector<char>::iterator end_pos);

		// @NOTE: not in use in this implementation
		size_t try_reading(std::vector<char>& main_buff,
				tcp::socket& socket,
				size_t payload_size);


	private:
		boost::asio::io_service _io_service;

		std::string _host_url;
		std::string _file_path;
		std::string _checksum;

		int _file_size;

		std::atomic<int> _offset;

		const int _NUM_THREADS;

		std::fstream _dest_file;

		std::vector<std::thread> _threads;

		// @NOTE:
		// members not in use for this implementation
		//
		std::condition_variable _file_cv;
		std::mutex    _file_lock;
		ConcQueue<ByteRange> _tasks;

};

#endif
