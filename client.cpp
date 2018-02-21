#include "client.h"

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

/*
 * Intended for small messages, will probably not be used here.
 */
std::string try_reading_from_sock(tcp::socket& sock)
{
	std::array<char, BUFF_SIZE> buff = { };
	boost::system::error_code ec;
	sock.read_some(boost::asio::buffer(buff), ec); //@TODO: take care of ec
	std::string msg(buff.data());
	return msg;
}

/*
 */
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

/*
 * for finding "\r\n\r\n" in an array, which denotes the end of a http header.
 * Thus, everything after can be written to a file.
 *
 * In case where it's not found, returns buff.end() - 3
 */
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

void simple_download(std::ofstream& outfile, tcp::socket& socket, std::vector<char>& buff,
		size_t len, std::vector<char>::iterator it)
{
	size_t i = 0;
	for (; it != buff.end() && i < len; ++it, ++i)
		outfile << *it;

	// lazy solution @TODO: grab content-length and parse that instead of
	// reading until none left
	//int content_length = parse_for_cont_length(header);
	boost::system::error_code ec;
	for (;;) {
		for (char& c : buff)
			c = '\0';
		len = socket.read_some(boost::asio::buffer(buff), ec);
		if (ec == boost::asio::error::eof)
			break;
		else if (ec)
			throw boost::system::system_error(ec);
		for (size_t i = 0; i < len; i++)
			outfile << buff[i];
	}
}

void parallel_download()
{
}

void run(std::string url, std::string filepath)
{
	tcp::socket socket = connect_to_server(url);

	HttpRequest req (url, filepath);
	try_writing_to_sock(socket, req.to_string());

	// open file, read from socket and write to file
	std::ofstream outfile;
	auto temp = split_(filepath, "/");
	std::string file_name = temp.back();
	outfile.open(file_name, std::ios::out | std::ios::binary);

	std::vector<char> buff(BUFF_SIZE, '\0');
	boost::system::error_code ec;
	size_t len = socket.read_some(boost::asio::buffer(buff), ec);

	auto crlf_pos = find_crlfsuffix_in(buff); // note: position of CRLFCRLF
	auto not_found = buff.end() - 3;
	if (crlf_pos == not_found)
		throw "crlf not in initial http response";

	std::string header (buff.begin(), crlf_pos);
	bool accepts_byte_ranges = check_accepts_byte_ranges(header);
	size_t body_len = len - header.length() - 4; // 4 bc CRLFCRLF is 4 characters long
	auto body_pos = crlf_pos + 4; // 4 bc CRLFCRLF is 4 characters long
	if (!accepts_byte_ranges)
		simple_download(outfile, socket, buff, body_len, body_pos);
	else
		parallel_download();

	// @TODO: use accepts_byte_ranges to determine if requesting chunk or not
	//outfile << std::string(crlf_pos + 4, buff.end());
	// check if content_length equals amount grabbed thus far.
	// If not, then use voodoo magic.
	// @TODO: update loop to record amount_downloaded, and compare to content_length
	// and if not equal, make an HTTP request for more.
	// Although, this seems like if Byte-Range didn't work, then this definitely wouldn't
	// work.

	outfile.close();
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

		run(url_str, filename_str);

	} catch(boost::system::system_error& e) {
		std::cerr << e.what() << "\n";
	} catch(std::exception& e) {
		std::cerr << e.what() << "\n";
	}

	return 0;
}
