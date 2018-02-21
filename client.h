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

#include "HttpRequest.h"

#define SMALL_BUFF_SIZE 128 // @TODO: make large buff_size for when reading actual data
//#define BUFF_SIZE 1024
#define BUFF_SIZE 4096

using String_Deq = std::deque<std::string>;

using boost::asio::ip::tcp;

std::deque<std::string> split_(std::string full_msg, std::string delim);

/*
 * if li = ["abc", "def", "ghi"]
 * then a call with delim = "/" will result in:
 * "/abc/def/ghi
 */
std::string join_(std::deque<std::string> msgs, std::string delim);

#endif
