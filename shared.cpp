#include "shared.h"

std::deque<std::string> split_(std::string full_msg, std::string delim)
{
	std::deque<std::string> msgs;
	std::size_t offset = 0;
	std::size_t endpos = 0;
	std::size_t len    = 0;

	do {
		endpos = full_msg.find(delim, offset);
		std::string temp;

		if (endpos != std::string::npos) {
			len = endpos - offset;
			temp = full_msg.substr(offset, len);
			msgs.push_back(temp);

			offset = endpos + delim.size();
		} else {
			temp = full_msg.substr(offset);
			msgs.push_back(temp);
			break;
		}
	} while (endpos != std::string::npos);
	return msgs;
}

std::string join_(std::deque<std::string> msgs, std::string delim)
{
	std::string full_msg;
	for (std::string msg : msgs) {
		full_msg += delim + msg;
	}
	return full_msg;
}

std::string try_reading_from_sock(tcp::socket& sock)
{
	std::array<char, BUFF_SIZE> buff = { };
	boost::system::error_code ec;
	sock.read_some(boost::asio::buffer(buff), ec); //@TODO: take care of ec
	std::string msg(buff.data());
	return msg;
}

void try_writing_to_sock(tcp::socket& sock, std::string msg)
{
	boost::system::error_code ec;
	boost::asio::write(sock, boost::asio::buffer(msg),
		       boost::asio::transfer_all(), ec);
}

tcp::socket connect_to_server(std::string url)
{
	// establish connection to server
	boost::asio::io_service io_service;
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(url, "http");

	// test for both ipv4 and ipv6 ip's
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	tcp::socket socket(io_service);
	boost::system::error_code ec = boost::asio::error::host_not_found;
	while (ec &&  endpoint_iterator != end) {
		socket.close();
		socket.connect(*endpoint_iterator++, ec);
	}
	if (ec)
		throw boost::system::system_error(ec);
	return socket;
}

std::vector<char>::iterator find_crlfsuffix_in(std::vector<char>& buff)
{
	auto it = buff.begin();
	for (; it != buff.end() - 3; ++it) {
		auto temp = it;
		if (*temp++ != '\r')
			continue;
		if (*temp++ != '\n')
			continue;
		if (*temp++ != '\r')
			continue;
		if (*temp != '\n')
			continue;
		else
			break;
	}
	if (it == buff.end() - 3)
		it = buff.end();
	return it;
}

bool check_accepts_byte_ranges(std::string httpHeader)
{
	std::size_t start_pos = httpHeader.find("Accept-Ranges:");
	if (start_pos == std::string::npos)
		return false;
	std::size_t crlf_pos = httpHeader.find("\r\n", start_pos);
	if (crlf_pos == std::string::npos)
		throw "didn't give correct httpHeader";
	std::size_t byte_pos = httpHeader.find("bytes", start_pos);
	std::size_t Byte_pos = httpHeader.find("Bytes", start_pos);
	if (byte_pos < crlf_pos || Byte_pos < crlf_pos)
		return true;
	return false;
}

int parse_for_cont_length(std::string httpHeader)
{
	std::size_t start_pos = httpHeader.find("Content-Length:");
	if (start_pos == std::string::npos)
		throw "no content length in httpHeader";
	std::size_t crlf_pos = httpHeader.find("\r\n", start_pos);
	std::string temp ("Content-Length:");
	std::string num = httpHeader.substr(start_pos + temp.length(), crlf_pos);
	auto end_pos = std::remove(num.begin(), num.end(), ' ');
	num.erase(end_pos, num.end());
	return std::stoi(num);
}

