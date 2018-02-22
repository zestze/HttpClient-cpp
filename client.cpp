#include "client.h"

void Client::simple_download(std::ofstream& outfile, std::vector<char>& buff,
		size_t len, std::vector<char>::iterator it)
{
	tcp::socket& socket = *_sockptr;
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

void Client::parallel_download(std::ofstream& outfile, std::vector<char>& buff,
		size_t len, std::vector<char>::iterator it)
{
}

void Client::run()
{
	_sockptr = std::make_unique<tcp::socket>(connect_to_server(_host_url));
	tcp::socket& socket = *_sockptr;

	HttpRequest req (_host_url, _file_path);
	try_writing_to_sock(socket, req.to_string());

	// open file, read from socket and write to file
	std::ofstream outfile;
	auto temp = split_(_file_path, "/");
	std::string file_name = temp.back();
	outfile.open(file_name, std::ios::out | std::ios::binary);

	std::vector<char> buff(BUFF_SIZE, '\0');
	boost::system::error_code ec;
	size_t len = socket.read_some(boost::asio::buffer(buff), ec);

	auto crlf_pos = find_crlfsuffix_in(buff); // note: position of CRLFCRLF
	if (crlf_pos == buff.end())
		throw "crlf not in initial http response";

	std::string header (buff.begin(), crlf_pos);
	bool accepts_byte_ranges = check_accepts_byte_ranges(header);
	size_t body_len = len - header.length() - 4; // 4 bc CRLFCRLF is 4 characters long
	auto body_pos = crlf_pos + 4; // 4 bc CRLFCRLF is 4 characters long
	if (!accepts_byte_ranges)
		simple_download(outfile, buff, body_len, body_pos);
	else
		parallel_download(outfile, buff, body_len, body_pos);

	outfile.close();
}

