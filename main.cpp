#include "client.h"
#include "shared.h"

int main(int argc, char **argv)
{
	std::ios_base::sync_with_stdio(false);
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <url>\n";
		return -1;
	}
	try {
		// argument formatting
		std::deque<std::string> parts = split_(argv[1], "/");

		std::string url_str = parts[0];
		parts.pop_front();

		std::string filepath_str = join_(parts, "/");

		Client client(url_str, filepath_str);
		client.run();

	} catch(boost::system::system_error& e) {
		std::cerr << e.what() << "\n";
	} catch(std::exception& e) {
		std::cerr << e.what() << "\n";
	}

	return 0;

}