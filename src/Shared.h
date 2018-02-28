#ifndef __SHARED_H__
#define __SHARED_H__

// @NOTE:
// These functions serve as a sort of library for the
// client. The are common functions that don't explicitly need
// to be included in the Client source files.
//
// I avoided making an actual library, since the library isn't that
// full of functions, nor is the task that large.
//

#include <boost/asio.hpp>
#include <string>
#include <array>
#include <vector>
#include <ostream>
#include <fstream>
#include <map>
#include <iostream>
#include <cstdint>

#ifndef BUFF_SIZE
#define BUFF_SIZE 4096
#endif

#ifndef POISON
#define POISON -2
#endif

using boost::asio::ip::tcp;

namespace Shared {

	// @NOTE:
	// if str = "/abc/def/ghi"
	// then a call with delim = "/" will result in:
	// ["abc", "def", "ghi"]
	//
	std::vector<std::string> split(std::string full_msg, std::string delim);

	// @NOTE:
	// if li = ["abc", "def", "ghi"]
	// then a call with delim = "/" will result in:
	// "/abc/def/ghi
	//
	std::string join(std::vector<std::string> msgs, std::string delim);


	// @NOTE:
	// Intended for small messages, will probably not be used here.
	//
	std::string try_reading_msg(tcp::socket& sock);

	// @NOTE:
	// simply takes a msg and writes it to a socket
	//
	void try_writing(tcp::socket& sock, std::string msg);

	// @NOTE:
	// connects to a server, performs dns resolution, and tests for
	// ipv4 and ipv6 connections.
	//
	tcp::socket connect_to_server(std::string url, boost::asio::io_service& io_service);

	// @NOTE:
	// for finding "\r\n\r\n" in an array, which denotes the end of a http header.
	// Thus, everything after can be written to a file.
	//
	// In case where it's not found, returns buff.end()
	//
	std::vector<char>::iterator find_crlfsuffix_in(std::vector<char>& buff);

	// @NOTE:
	// function name is descriptive
	//
	bool check_accepts_byte_ranges(std::string httpHeader);

	// @NOTE:
	// function name is descriptive
	//
	int parse_for_cont_length(std::string httpHeader);


	// @NOTE:
	// for grabbing string representation of crc32 check_sum
	// if x-goog-hash field has value 'crc32c=asdfasdf=='
	// then this will return 'asdfasdf=='
	//
	// will not work when given multiple hashes in same field.
	// hashes must be in separate header fields.
	//
	// if not found, returns _HASH_NOT_FOUND
	//
	std::string parse_for_crc32(std::string httpHeader);

	// @NOTE:
	// similar to cdc32, but for md5
	//
	std::string parse_for_md5(std::string httpHeader);

	// @NOTE:
	// underlying implementation for
	// parse_for_crc32
	// and
	// parse_for_md5
	//
	std::string parse_for_hash(std::string httpHeader, std::string hash_tag);

	// @NOTE:
	// return value of the parse family functions when hash is not found
	//
	const std::string _HASH_NOT_FOUND ("_HASH_NOT_FOUND");

	// @TODO: replace with template
	void write_to_file(size_t amount_to_write,
			std::vector<char>::iterator start_pos,
			std::vector<char>::iterator end_pos,
			std::ofstream& outfile);

	void write_to_file(size_t amount_to_write,
			std::vector<char>::iterator start_pos,
			std::vector<char>::iterator end_pos,
			std::fstream& outfile);

	// @NOTE:
	// currently only works for md5 base64 numbers.
	//
	// Adjusts for little endian order
	//
	std::string convert_base64_to_hex(std::string base64_num);

	// @NOTE:
	// used by above function to perform conversion
	//
	const std::map<int, char> _INT_TO_CHAR {
			{0, '0'}, {1, '1'}, {2, '2'}, {3, '3'},
			{4, '4'}, {5, '5'}, {6, '6'}, {7, '7'},
			{8, '8'}, {9, '9'}, {10, 'A'}, {11, 'B'},
			{12, 'C'}, {13, 'D'}, {14, 'E'}, {15, 'F'}
	};

	// @NOTE:
	// used by above function to perform conversion
	//
	const std::map<char, std::uint8_t> _BASE64_TABLE = {
		{'=', 0}, {'A', 0}, {'B', 1}, {'C', 2}, {'D', 3}, {'E', 4},
		{'F', 5}, {'G', 6}, {'H', 7}, {'I', 8}, {'J', 9}, {'K', 10},
		{'L', 11}, {'M', 12}, {'N', 13}, {'O', 14}, {'P', 15},
		{'Q', 16}, {'R', 17}, {'S', 18}, {'T', 19}, {'U', 20},
		{'V', 21}, {'W', 22}, {'X', 23}, {'Y', 24}, {'Z', 25},
		{'a', 26}, {'b', 27}, {'c', 28}, {'d', 29}, {'e', 30}, {'f', 31},
		{'g', 32}, {'h', 33}, {'i', 34}, {'j', 35}, {'k', 36}, {'l', 37},
		{'m', 38}, {'n', 39}, {'o', 40}, {'p', 41}, {'q', 42}, {'r', 43},
		{'s', 44}, {'t', 45}, {'u', 46}, {'v', 47}, {'w', 48}, {'x', 49},
		{'y', 50}, {'z', 51}, {'0', 52}, {'1', 53}, {'2', 54}, {'3', 55},
		{'4', 56}, {'5', 57}, {'6', 58}, {'7', 59}, {'8', 60}, {'9', 61},
		{'+', 62}, {'/', 63}
	};


}

#endif
