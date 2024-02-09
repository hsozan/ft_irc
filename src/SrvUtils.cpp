#include "../include/Server.hpp"

void ErrorLogger( string messageInfo, const char* fileInfo, int lineInfo, bool isFatal)
{
	ofstream errorLog;

	errorLog.open("error.log", ios::app);
	errorLog << "Error Time: " << __TIME__ << endl;
	errorLog << "Error in file: " << fileInfo << ":" << lineInfo << endl;
	errorLog << "Error info: " << string(strerror(errno)) << endl;
	errorLog << "----------------------------------------" << endl;
	errorLog.close();
	if (isFatal) {
		throw runtime_error(messageInfo);
	}
	std::cout << messageInfo << std::endl;
}

void log(const string& message) {
	char buffer[100];
	time_t currentTime = time(NULL);
	ofstream logFile;

	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&currentTime));
	string timeStr(buffer);

	logFile.open("log.log", std::ios::app);
	logFile << timeStr << " " << message << std::endl;
	logFile.close();

	std::cout << "\033[0;34m[" << timeStr << "]\033[0m " << message << std::endl;
}

void Server::processPartialCommands(int clientSocketFD)
{
	string& clientBuffer = clientBuffers[clientSocketFD].getBuffer();
	size_t endOfCommand;
	string command;

	if (clientBuffer[0] == '/')
	{
		while ((endOfCommand = clientBuffer.find('\n')) != string::npos)
		{
			command = clientBuffer.substr(0, endOfCommand);
			clientBuffer.erase(0, endOfCommand + 1);
			CommandParser::commandParser(command.c_str(), _clients[clientSocketFD], this);
		}
		if (!clientBuffer.find('\n'))
			clientBuffer.clear();
	}

	else
	{
		while ((endOfCommand = clientBuffer.find("\r\n")) != string::npos)
		{
			command = clientBuffer.substr(0, endOfCommand);
			clientBuffer.erase(0, endOfCommand + 2);
			CommandParser::commandParser(command.c_str(), _clients[clientSocketFD], this);
		}
		clientBuffer.clear();
	}
}

void Server::handleClient(int clientSocketFD)
{
		const size_t BUFFER_SIZE = 512;
		char tempBuffer[BUFFER_SIZE];
		memset(tempBuffer, 0, BUFFER_SIZE);

		ssize_t received = recv(clientSocketFD, tempBuffer, BUFFER_SIZE - 1, 0);
		if (received > 0) {
			clientBuffers[clientSocketFD].appendtoBuffer(string(tempBuffer, received));
			cout << "Received: " << tempBuffer << endl;
			processPartialCommands(clientSocketFD);
		} else if (received == 0 || errno == ECONNRESET) {
			FD_CLR(clientSocketFD, &read_set);
			clientDisconnect(clientSocketFD);
			clientBuffers.erase(clientSocketFD);
		} else {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				FD_CLR(clientSocketFD, &read_set);
				ErrorLogger("recv error", __FILE__, __LINE__, false);
				close(clientSocketFD);
				clientBuffers.erase(clientSocketFD);
			}
		}
}

void Server::clientDisconnect(int clientSocketFD)
{
     try
    {
        std::map<int, Client*>::iterator it = _clients.find(clientSocketFD);
        if (it == _clients.end()) {
            error(RED, "Client not found in client map.\n", RESET);
            return;
        }
        removeClientFromAllChannels(it->second);
        it->second->leave();
        ostringstream messageStreamDisconnect;
        messageStreamDisconnect << "Client " << it->second->getNickName() << " has disconnected.";
        log(messageStreamDisconnect.str());
        FD_CLR(clientSocketFD, &read_set);
        close(clientSocketFD);
        delete it->second;
        _clients.erase(it);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}

void Server::setSrvPass(const string& pass) {
	_serverPass = pass;
}

bool Server::verifySrvPass(const string& pass) {
	if (_serverPass == pass) {
		return true;
	}
	return false;
}

void Server::signalHandler(int signum)
{
	Server::getInstance()->shutdownSrv();
	exit(signum);
}

void Server::shutdownSrv()
{
	std::cout << "Shutting down server..." << std::endl;
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		Client* client = it->second;
		if (client != NULL) {
			client->sendMessage("ERROR :Closing Link: " + client->getHostName() + " (Server shutdown)");
			removeClientFromAllChannels(client);
			close(it->first);
			delete client;
		}
	}
	_clients.clear();
	if (_serverSocketFD != -1) {
		close(_serverSocketFD);
		_serverSocketFD = -1;
	}
	if (_bot != NULL) {
		delete _bot;
		_bot = NULL;
	}
	FD_ZERO(&read_set);
	std::cout << "Server shutdown complete." << std::endl;
}

int error(const char *msg, const char *err, const char *err2)
{
    if(msg)
        std::cerr << msg;
    if(err)
        std::cerr << err;
    if(err2)
        std::cerr << err2;
    std::cerr << std::endl;
	exit(1);
	return 1;
}
