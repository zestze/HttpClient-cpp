#include "shared.h"

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
	for (std::string msg : msgs) {
		full_msg += delim + msg;
	}
	return full_msg;
}

std::string Shared::try_reading_msg(tcp::socket& sock)
{
	std::array<char, BUFF_SIZE> buff = { };
	boost::system::error_code ec;
	sock.read_some(boost::asio::buffer(buff), ec); //@TODO: take care of ec
	std::string msg(buff.data());
	return msg;
}

void Shared::try_writing(tcp::socket& sock, std::string msg)
{
	boost::system::error_code ec;
	boost::asio::write(sock, boost::asio::buffer(msg),
		       boost::asio::transfer_all(), ec);
}

tcp::socket Shared::connect_to_server(std::string url, boost::asio::io_service& io_service)
{
	// establish connection to server
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
		throw "didn't give correct httpHeader";
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
		throw "no content length in httpHeader";
	std::size_t crlf_pos = httpHeader.find("\r\n", start_pos);
	std::string content_length ("Content-Length:");
	std::string num = httpHeader.substr(start_pos + content_length.length(),
			crlf_pos - start_pos - content_length.length());
	auto end_pos = std::remove(num.begin(), num.end(), ' ');
	num.erase(end_pos, num.end());
	return std::stoi(num);
}

std::string Shared::parse_for_cdc32(std::string httpHeader)
{
	std::vector<std::size_t> sp_vec;
	std::string x_goog_hash ("x-goog-hash:");
	std::size_t start_pos = httpHeader.find(x_goog_hash);
	if (start_pos == std::string::npos)
		throw "no hash in httpHeader";
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
		std::string crc_tag ("crc32c=");
		auto new_start = hash.find(crc_tag);
		if (new_start == std::string::npos)
			continue;
		return hash.substr(new_start + crc_tag.length(),
				hash.length() - crc_tag.length());
	}
	return _CRC_HASH_NOT_FOUND;
}

void Shared::write_to_file(size_t amount_to_write, std::vector<char>::iterator start_pos,
		std::vector<char>::iterator end_pos, std::ofstream& outfile)
{
	for (size_t i = 0; i < amount_to_write; i++) {
		if (start_pos == end_pos)
			throw std::string("start_pos == end_pos");
		outfile << *start_pos++;
	}
}

void Shared::write_to_file(size_t amount_to_write, std::vector<char>::iterator start_pos,
		std::vector<char>::iterator end_pos, std::fstream& outfile)
{
	for (size_t i = 0; i < amount_to_write; i++) {
		if (start_pos == end_pos)
			throw std::string("start_pos == end_pos");
		outfile << *start_pos++;
	}
}

std::string Shared::convert_base64_to_hex(std::string base64_num)
{
	std::string bin_num ("");
	for (char c : base64_num) {
		if (c == '=') {
			continue;
		}
		else {
			//int d = _BASE64_TABLE[c];
			int d = _BASE64_TABLE.at(c);
			std::string bin = std::bitset<sizeof(int)>(d).to_string();
			bin_num += bin;
		}
	}

	std::vector<int> hex_digits = binStr_to_hexVec(bin_num);
	std::string hexStr = hexVec_to_hexStr(hex_digits);
	return hexStr;
	//return bin_num;
	/*
	long long ll_bin_num = stoll(bin_num, nullptr, 2);

	std::stringstream ss;
	ss << std::hex << ll_bin_num;
	//std::cout << bin_num << "\n";
	return ss.str();
	*/
}

std::vector<int> Shared::binStr_to_hexVec(std::string bin_num)
{
	std::vector<int> hex_digits;
	for (auto rpos = bin_num.rbegin(); rpos != bin_num.rend(); ) {
		auto temp = rpos;
		int hex_dig = 0;
		for (int bits = 0; bits < 4 && rpos != bin_num.rend();
								++bits, ++temp) {
			int power = 1;
			for (int i = 0; i < bits; i++)
				power *= 2;
			if (*temp == '1')
				hex_dig += power;
		}
		hex_digits.push_back(hex_dig);
		rpos = temp;
	}
	std::reverse(hex_digits.begin(), hex_digits.end());
	return hex_digits;
}

std::string Shared::hexVec_to_hexStr(std::vector<int> hexVec)
{
	std::string hexStr ("");

	for (int digit : hexVec)
		hexStr += _INT_TO_CHAR.at(digit);

	return hexStr;
}

std::string Shared::convert_base64_2(std::string base64_num)
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

	std::array<int, 32> bits32;
	for (int i = 0; i < 32; i++) {
		bits32[i] = base64_in_bin[i];
	}

	std::vector<int> hex_digits;
	const int num_bytes = bits32.size() / 8;
	for (int i = 0; i < num_bytes; i++) {
	}

	// convert base10 digits, to binary digits
	//
	// convert binary digits, to hexadecimal
}
