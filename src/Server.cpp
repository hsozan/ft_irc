#include "../include/Server.hpp"

Server* Server::ins = NULL;

Server::Server(int serverSocketFamily, int serverSocketProtocol, int serverSocketPort, string serverName)
	: _serverSocketFD(-1),
	_serverSocketFamily(serverSocketFamily),
	_serverSocketProtocol(serverSocketProtocol),
	_serverSocketPort(serverSocketPort),
	_serverName(serverName),
	_serverPass(""),
	_bot(NULL)
{
	
	Server::setInstance(this);

	memset(&serverAddress, 0, sizeof(serverAddress));

	FD_ZERO(&read_set);
}

Server::~Server()
{
	delete Server::ins;
	Server::ins = NULL;

	for (map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		delete it->second;
	}
	_clients.clear();
	for (map<string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
		delete it->second;
	}
	_channels.clear();
	if (_serverSocketFD != -1)
		close(_serverSocketFD);
	if (_bot != NULL)
		delete _bot;
	FD_ZERO(&read_set);
}

void Server::socketStart()
{
	_serverSocketFD = socket(_serverSocketFamily, _serverSocketProtocol, 0);

	if (_serverSocketFD == -1)
		ErrorLogger(FAILED_SOCKET, __FILE__, __LINE__, false);
	if (fcntl(_serverSocketFD, F_SETFL, O_NONBLOCK) == -1)
	{
		close(_serverSocketFD);
		ErrorLogger(FAILED_SOCKET_NONBLOCKING, __FILE__, __LINE__, false);
	}

	int reuse = 1;
	if (setsockopt(_serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
	{
		close(_serverSocketFD);
		ErrorLogger(FAILED_SOCKET_OPTIONS, __FILE__, __LINE__, false);
	}
}

void Server::socketInit()
{
	switch (_serverSocketFamily)
	{
		case AF_INET:
			serverAddress.sin_addr.s_addr = INADDR_ANY;
			serverAddress.sin_family = _serverSocketFamily;
			serverAddress.sin_port = htons(_serverSocketPort);
			memset(&(serverAddress.sin_zero), '\0', 8);
			break;

		default:
			close(_serverSocketFD);
			ErrorLogger(FAILED_SOCKET_DOMAIN, __FILE__, __LINE__, false);
	}
}

void Server::socketBind()
{
	if (::bind(_serverSocketFD, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1)
	{
		close(_serverSocketFD);
		ErrorLogger(FAILED_SOCKET_BIND, __FILE__, __LINE__, false);
		exit(1);
	}
}


void Server::socketListen()
{
	if (listen(_serverSocketFD, BACKLOG_SIZE) == -1 )
	{
		close(_serverSocketFD);
		ErrorLogger(FAILED_SOCKET_LISTEN, __FILE__, __LINE__, false);
	}
	FD_SET(_serverSocketFD, &read_set);
}


int Server::socketAccept()
{
	struct sockaddr_storage clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);
	int clientSocketFD = accept(_serverSocketFD, (struct sockaddr *)&clientAddress, &clientAddressLength);

	if (clientSocketFD == -1)
	{
		ErrorLogger(FAILED_SOCKET_ACCEPT, __FILE__, __LINE__, true);
		return -1;
	}
	if (fcntl(clientSocketFD, F_SETFL, O_NONBLOCK) == -1)
	{
		close(clientSocketFD);
		ErrorLogger(FAILED_SOCKET_NONBLOCKING, __FILE__, __LINE__, false);
	}

	int reuse = 1;

	if (setsockopt(clientSocketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
	{
		close(clientSocketFD);
		ErrorLogger(FAILED_SOCKET_OPTIONS, __FILE__, __LINE__, false);
	}

	char hostname[NI_MAXHOST];

	if (inet_ntop(AF_INET, &(((struct sockaddr_in *)&clientAddress)->sin_addr), hostname, sizeof(hostname)) == NULL)
	{
		ErrorLogger(FAILED_SOCKET_GETADDRINFO, __FILE__, __LINE__, true);
	}

	Client* client = NULL;

	client = new Client(clientSocketFD, ntohs(((struct sockaddr_in*)&clientAddress)->sin_port), hostname, _serverName);

	_clients.insert(std::make_pair(clientSocketFD, client));

	std::ostringstream messageStream;
	messageStream << "New Client: has connected.";
	client->sendMessage("Connected to Server");
	client->sendMessage("Enter the server password using /PASS");

	log(messageStream.str());
	return clientSocketFD;
}

void Server::serverRun()
{
	socketStart();
	socketInit();
	socketBind();
	socketListen();
	signal(SIGINT, signalHandler);
	try
	{
		_bot = new Bot("localhost", _serverSocketPort, _serverPass);
	}
	catch (const std::exception &e)
	{
		delete _bot;
		_bot = NULL;
		std::cout << e.what() << std::endl;
	}

	for (;;)
	{
		int max_fd = _bot->getSocket();
		int n = 0;
		FD_ZERO(&read_set);
		FD_SET(_serverSocketFD, &read_set);
		FD_SET(_bot->getSocket(), &read_set);
		for (map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
		{
			FD_SET((*it).second->getClientSocketFD(), &read_set);
			max_fd = std::max(max_fd, (*it).second->getClientSocketFD());
		}
		n = select(max_fd + 1, &read_set, NULL, NULL, NULL);
		if (n)
		{
			if (FD_ISSET(_serverSocketFD, &read_set))
				socketAccept();
			for (map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
			{
				if (FD_ISSET((*it).second->getClientSocketFD(), &read_set))
				{
					handleClient((*it).second->getClientSocketFD());
					break;
				}
			}
			if (FD_ISSET(_bot->getSocket(), &read_set))
				_bot->listen();
		}
	}
}
