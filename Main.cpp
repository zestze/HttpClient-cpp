#include "Client.h"
#include "shared.h"

int main(int argc, char **argv)
{
	std::ios_base::sync_with_stdio(false);
	if (argc != 5) {
		std::cerr << "Usage: " << argv[0] << "";
		std::cerr << " <url> <chunk-size> <num-threads> <force-simple>\n";
		std::cerr << "<force-simple> should be a 1 or 0\n";
		return -1;
	}
	try {
		// argument formatting
		std::vector<std::string> parts = split_(argv[1], "/");

		std::string url_str = parts[0];
		parts.erase(parts.begin());

		std::string filepath_str = join_(parts, "/");

		int chunk_size = std::stoi(argv[2]);
		int num_threads = std::stoi(argv[3]);
		bool force_simple;
		if (std::stoi(argv[4]) > 0)
			force_simple = true;
		else
			force_simple = false;

		Client client(url_str, filepath_str, chunk_size, num_threads);
		client.run(force_simple);

	} catch(boost::system::system_error& e) {
		std::cerr << e.what() << "\n";
	} catch(std::exception& e) {
		std::cerr << e.what() << "\n";
	}

	return 0;

}
