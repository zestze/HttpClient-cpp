// @NOTE:
// the Client class is the meat of the
// application. The class is grouped together
// like such to provide ease of development.
// The Client objects aren't meant to be passed or copied,
// its meant to compactly and efficiently gather variables
// and functions that are semantically grouped.
//
#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <limits>
#include <cstdio>
//#include <boost/asio.hpp>
#include <asio.hpp>
#include <unistd.h>
#include <sys/wait.h>

#include "HttpRequest.h"
#include "Shared.h"
#include "ByteRange.h"

#ifndef BUFF_SIZE
#define BUFF_SIZE 4096
#endif

// for referencing 'seconds' as a 's' literal
//
using namespace std::chrono_literals;

// for boost asio socket functions
//
//using boost::asio::ip::tcp;
using tcp = asio::ip::tcp;

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
			auto temp = Shared::split(_file_path, "/");
			std::string file_name = temp.back();
			_dest_file_path = "../" + file_name;
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

		// @NOTE:
		// 'Entry' point for the Client class. Here the initial httpRequest
		// is sent, and it is decided to perform the simple download
		// or the concurrent download
		//
		// force_simple is an argument that can be passed, and as the
		// name suggests, will force the program to use the simple
		// download regardless if the server supports ByteRange
		//
		void run(bool force_simple = false);

		void simple_download();

		void parallel_download();

		void worker_thread_run(ByteRange br, int ID);

		// @NOTE:
		// Calculates md5sum hash and compares it to md5hash from
		// http header.
		//
		// Uses md5sum linux utility, which the program assumes is
		// stored in /usr/bin/md5sum
		//
		// The calculated md5sum is adjusted according to my machine
		// being little endian and the sent value being big endian.
		//
		// Returns true if equal, or if there is no md5hash stored,
		// false otherwise
		//
		bool check_sum();

		// @NOTE: not in use in this implementation
		size_t try_reading(std::vector<char>& main_buff,
				tcp::socket& socket,
				size_t payload_size);

	private:
		asio::io_service _io_service;

		std::string _host_url;
		std::string _file_path;
		std::string _dest_file_path;
		std::string _checksum;

		int _file_size;

		std::atomic<int> _offset;

		const int _NUM_THREADS;

		std::fstream _dest_file;

		std::vector<std::thread> _threads;
};

#endif
