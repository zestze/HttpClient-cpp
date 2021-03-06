#ifndef __HTTPREQUEST_H__
#define __HTTPREQUEST_H__

#include <string>

// @NOTE:
// simple class for creating httpRequests quickly and easily
//
class HttpRequest {
	public:
		HttpRequest(std::string host, std::string file_path)
		{
			_get_or_head = "GET ";
			_host = host;
			_file_path = file_path;
			_connection_type = "close";
			_accept = std::string("text/html,") + "application/xhtml"
				+ "+xml,application/xml;q=0.9,image/webp,image/"
				+ "apng,*/*;q=0.8";
			_accept_encoding = std::string("gzip, deflate");
			_accept_lang = std::string("en-US,en;q=0.9");
			_range = "";
		}

		// don't want anyone to use default constructor for now
		HttpRequest() =  delete;

		void simplify_accept() { _accept = "*/*"; }
		void set_keepalive()   { _connection_type = "keep-alive"; }
		void set_close()       { _connection_type = "close"; }
		void set_head()	       { _get_or_head = "HEAD "; }

		void set_range(int start, int end)
		{
			_range = "bytes=" + std::to_string(start)
				+ "-" + std::to_string(end);
		}

		std::string to_string()
		{
			return _get_or_head + _file_path + " HTTP/1.1\r\n"
				+ "Accept: " + _accept + "\r\n"
				+ "Accept-Encoding: " + _accept_encoding + "\r\n"
				+ "Accept-Language: " + _accept_lang + "\r\n"
				+ "Connection: " + _connection_type + "\r\n"
				+ "Host: " + _host + "\r\n"
				+ "Range: " + _range + "\r\n"
				+ "\r\n";
		}
	private:
		std::string _get_or_head;
		std::string _file_path;
		std::string _host;
		std::string _connection_type;
		std::string _range;
		std::string _accept;
		std::string _accept_encoding;
		std::string _accept_lang;
};

#endif
