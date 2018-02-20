/*
 */

#ifndef __TEST_H__
#define __TEST_H__

#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <deque>

using boost::asio::ip::tcp;

std::deque<std::string> split_(std::string full_msg, std::string delim);

std::string join_(std::deque<std::string> msgs, std::string delim);

#endif
