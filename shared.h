#ifndef __SHARED_H__
#define __SHARED_H__

#include <boost/asio.hpp>
#include <string>
#include <deque>
#include <array>

#ifndef BUFF_SIZE
#define BUFF_SIZE 4096
#endif

#ifndef POISON
#define POISON -2
#endif

using boost::asio::ip::tcp;

std::deque<std::string> split_(std::string full_msg, std::string delim);

// if li = ["abc", "def", "ghi"]
// then a call with delim = "/" will result in:
// "/abc/def/ghi
std::string join_(std::deque<std::string> msgs, std::string delim);


// Intended for small messages, will probably not be used here.
std::string try_reading_from_sock(tcp::socket& sock);

void try_writing_to_sock(tcp::socket& sock, std::string msg);

tcp::socket connect_to_server(std::string url);

// for finding "\r\n\r\n" in an array, which denotes the end of a http header.
// Thus, everything after can be written to a file.
//
//In case where it's not found, returns buff.end()
std::vector<char>::iterator find_crlfsuffix_in(std::vector<char>& buff);

bool check_accepts_byte_ranges(std::string httpHeader);

int parse_for_cont_length(std::string httpHeader);

#endif
