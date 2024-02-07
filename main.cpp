#include "include/Server.hpp"
#include "include/Client.hpp"
#include "include/Bot.hpp"

int main(int argc, char *argv[])
{
	//system("leaks Ircserv");
	if (argc != 3) 
		return error(RED, "Usage: ./server <port> <password>\n", RESET);
	int port = std::atoi(argv[1]);
	if (port <= 0 || port > 65535)
		return error(RED, "Invalid port number\n", RESET);
	std::string password = argv[2];
	try {
		Server	srv(AF_INET, SOCK_STREAM, port, "Gariban İRC");
		srv.setSrvPass(password);
		srv.serverRun();
	}
	catch (std::exception& e){
		return error(RED, e.what(), RESET);
	}
	return 0;
}
