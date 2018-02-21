#include "test.h"

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
		full_msg += msg + delim;
	}
	full_msg.pop_back(); // get rid of delim added on last iter of loop
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

void run(std::string url, std::string filepath)
{
	tcp::socket socket = connect_to_server(url);

	std::cout << try_reading_from_sock(socket) << "\n";
}

int main(int argc, char **argv)
{
	std::ios_base::sync_with_stdio(false);
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <url>\n";
		return -1;
	}
	try {
		// argument formatting
		std::deque<std::string> msgs = split_(argv[1], "/");

		std::string url_str = msgs[0];
		msgs.pop_front();

		std::string filename_str = join_(msgs, "/");
		// format: vimeo-test/...

		run(url_str, filename_str);
	} catch(std::exception& e) {
		std::cerr << e.what() << "\n";
	}

	return 0;
}
