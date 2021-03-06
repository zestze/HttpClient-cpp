#include "Shared.h"

std::vector<std::string> Shared::split(std::string full_msg, std::string delim)
{
	std::vector<std::string> msgs;
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

std::string Shared::join(std::vector<std::string> msgs, std::string delim)
{
	std::string full_msg;
	for (const std::string& msg : msgs) {
		full_msg += delim + msg;
	}
	return full_msg;
}

std::string Shared::try_reading_msg(tcp::socket& sock)
{
	std::array<char, BUFF_SIZE> buff = { };
	asio::error_code ec;
	sock.read_some(asio::buffer(buff), ec); //@TODO: take care of ec
	std::string msg(buff.data());
	return msg;
}

void Shared::try_writing(tcp::socket& sock, std::string msg)
{
	asio::error_code ec;
	asio::write(sock, asio::buffer(msg),
		       asio::transfer_all(), ec);
}

tcp::socket Shared::connect_to_server(std::string url, asio::io_service& io_service)
{
	// establish connection to server
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(url, "http");

	// extra for both ipv4 and ipv6 ip's
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	tcp::resolver::iterator end;

	tcp::socket socket(io_service);
	asio::error_code ec = asio::error::host_not_found;
	while (ec &&  endpoint_iterator != end) {
		socket.close();
		socket.connect(*endpoint_iterator++, ec);
	}
	if (ec)
		throw asio::system_error(ec);
	return socket;
}

std::vector<char>::iterator Shared::find_crlfsuffix_in(std::vector<char>& buff)
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

bool Shared::check_accepts_byte_ranges(std::string httpHeader)
{
	std::size_t start_pos = httpHeader.find("Accept-Ranges:");
	if (start_pos == std::string::npos)
		return false;
	std::size_t crlf_pos = httpHeader.find("\r\n", start_pos);
	if (crlf_pos == std::string::npos)
		throw std::runtime_error("didn't give correct httpHeader");
	std::size_t byte_pos = httpHeader.find("bytes", start_pos);
	std::size_t Byte_pos = httpHeader.find("Bytes", start_pos);
	if (byte_pos < crlf_pos || Byte_pos < crlf_pos)
		return true;
	return false;
}

int Shared::parse_for_cont_length(std::string httpHeader)
{
	std::size_t start_pos = httpHeader.find("Content-Length:");
	if (start_pos == std::string::npos)
		throw std::runtime_error("no content length in httpHeader");
	std::size_t crlf_pos = httpHeader.find("\r\n", start_pos);
	std::string content_length ("Content-Length:");
	std::string num = httpHeader.substr(start_pos + content_length.length(),
			crlf_pos - start_pos - content_length.length());
	auto end_pos = std::remove(num.begin(), num.end(), ' ');
	num.erase(end_pos, num.end());
	return std::stoi(num);
}

std::string Shared::parse_for_hash(std::string httpHeader, std::string hash_tag)
{
	std::vector<std::size_t> sp_vec;
	std::string x_goog_hash ("x-goog-hash:");
	std::size_t start_pos = httpHeader.find(x_goog_hash);
	if (start_pos == std::string::npos)
		throw std::runtime_error("no hash in httpHeader");
	while (start_pos != std::string::npos) {
		sp_vec.push_back(start_pos);
		start_pos = httpHeader.find(x_goog_hash, start_pos + 1);
	}

	for (std::size_t start_pos : sp_vec) {
		std::size_t crlf_pos = httpHeader.find("\r\n", start_pos);
		std::string hash = httpHeader.substr(
				start_pos + x_goog_hash.length(),
				crlf_pos - start_pos - x_goog_hash.length());
		auto new_end = std::remove(hash.begin(), hash.end(), ' ');
		hash.erase(new_end, hash.end());

		// 'crc32c=asdfasdfasdf==' is the format what we're looking for
		// or
		// 'md5=asdfasdfasdf==' is the format we're looking for
		auto new_start = hash.find(hash_tag);
		if (new_start == std::string::npos)
			continue;
		return hash.substr(new_start + hash_tag.length(),
				hash.length() - hash_tag.length());
	}
	return _HASH_NOT_FOUND;

}

std::string Shared::parse_for_crc32(std::string httpHeader)
{
	return parse_for_hash(httpHeader, "crc32=");
}

std::string Shared::parse_for_md5(std::string httpHeader)
{
	return parse_for_hash(httpHeader, "md5=");
}

void Shared::write_to_file(size_t amount_to_write, std::vector<char>::iterator start_pos,
		std::vector<char>::iterator end_pos, std::ofstream& outfile)
{
	for (size_t i = 0; i < amount_to_write; i++) {
		if (start_pos == end_pos)
			throw std::runtime_error("start_pos == end_pos");
		outfile << *start_pos++;
	}
}

void Shared::write_to_file(size_t amount_to_write, std::vector<char>::iterator start_pos,
		std::vector<char>::iterator end_pos, std::fstream& outfile)
{
	for (size_t i = 0; i < amount_to_write; i++) {
		if (start_pos == end_pos)
			throw std::runtime_error("start_pos == end_pos");
		outfile << *start_pos++;
	}
}

std::string Shared::inefficient_md5_hash(std::string file_name)
{
	std::ifstream file (file_name, std::ios::in | std::ios::binary);
	std::stringstream ss;
	ss << file.rdbuf();
	std::string contents = ss.str();
	unsigned char digest[MD5_DIGEST_LENGTH];
	MD5((unsigned char *)contents.c_str(), contents.size(),
			(unsigned char *)&digest);
	char mdString[33] = {'\0'};
	for (int i = 0; i < 16; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
	return mdString;
}

std::string Shared::convert_base64_to_hex(std::string base64_num)
{
	std::vector<std::uint8_t> digits_base10;
	for (char c : base64_num) {
		digits_base10.push_back(_BASE64_TABLE.at(c));
	}

	std::vector<int> base64_in_bin;
	std::array<int, 6> digit_bits; // @NOTE: only 1's and 0's allowed
	for (std::uint8_t digit : digits_base10) {

		for (auto& d : digit_bits)
			d = 0;

		if (digit / 32 > 0) {
			digit_bits[0] = 1;
			digit %= 32;
		}
		if (digit / 16 > 0) {
			digit_bits[1] = 1;
			digit %= 16;
		}
		if (digit / 8 > 0) {
			digit_bits[2] = 1;
			digit %= 8;
		}
		if (digit / 4 > 0) {
			digit_bits[3] = 1;
			digit %= 4;
		}
		if (digit / 2 > 0) {
			digit_bits[4] = 1;
			digit %= 2;
		}
		if (digit / 1 > 0)
			digit_bits[5] = 1;

		for (auto d : digit_bits)
			base64_in_bin.push_back(d);
	}

	// md5 is 128 bits long, crc32 was 32
	// all bits of number, in sequence
	std::array<int, 128> bits128;
	for (size_t i = 0; i < bits128.size(); i++) {
		bits128[i] = base64_in_bin[i];
	}

	std::vector<int> octet_digits_base10;
	const int num_octets = bits128.size() / 8;
	for (int i = 0; i < num_octets; i++) {
		int octet_digit = 0;
		if (bits128[i * 8 + 0] == 1)
			octet_digit += 128;
		if (bits128[i * 8 + 1] == 1)
			octet_digit += 64;
		if (bits128[i * 8 + 2] == 1)
			octet_digit += 32;
		if (bits128[i * 8 + 3] == 1)
			octet_digit += 16;
		if (bits128[i * 8 + 4] == 1)
			octet_digit += 8;
		if (bits128[i * 8 + 5] == 1)
			octet_digit += 4;
		if (bits128[i * 8 + 6] == 1)
			octet_digit += 2;
		if (bits128[i * 8 + 7] == 1)
			octet_digit += 1;

		octet_digits_base10.push_back(octet_digit);
	}

	std::vector<std::string> octet_digits_hex;
	for (int octet_digit_base10 : octet_digits_base10) {
		int lsd = octet_digit_base10 / 16;
		int msd = octet_digit_base10 % 16;

		std::string lsd_str (1, _INT_TO_CHAR.at(lsd));
		std::string msd_str (1, _INT_TO_CHAR.at(msd));

		//std::string hex_digit = msd_str + lsd_str;
		std::string hex_digit = lsd_str + msd_str;
		octet_digits_hex.push_back(hex_digit);
	}

	std::string retval;
	for (const std::string& s : octet_digits_hex)
		retval += s;

	return retval;
}
