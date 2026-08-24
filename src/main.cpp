#include "Server.hpp"
#include <iostream>
#include <cstdlib>

static bool isValidPort(const char* s) {
	for (int i = 0; s[i]; ++i) if (s[i] < '0' || s[i] > '9') return false;
	long p = std::strtol(s, NULL, 10);
	return (p >= 1024 && p <= 65535);
}

int main(int ac, char** av) {
	if (ac != 3) {
		std::cerr << "Usage: ./ircserv <port> <password>\n";
		return 1;
	}
	if (!isValidPort(av[1])) {
		std::cerr << "Invalid port\n";
		return 1;
	}
	try {
		Server s(std::atoi(av[1]), av[2]);
		s.initSocket();
		s.run();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
