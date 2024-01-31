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

// Partial komutları işler
// İstemci soket dosya tanımlayıcısını alır ve tamamlanmamış komutları işler.
void Server::processPartialCommands(int clientSocketFD)
{
	string& clientBuffer = clientBuffers[clientSocketFD].getBuffer();
	size_t endOfCommand;
	string command;

	// Komutlar '/' ile başlıyorsa
	if (clientBuffer[0] == '/')
	{
		while ((endOfCommand = clientBuffer.find('\n')) != string::npos)
		{
			command = clientBuffer.substr(0, endOfCommand);
			clientBuffer.erase(0, endOfCommand + 1);
			// Komutu işleyen CommandParser sınıfı kullanılır.
			CommandParser::commandParser(command.c_str(), _clients[clientSocketFD], this);
		}
		if (!clientBuffer.find('\n'))
			clientBuffer.clear();
	}

	else
	{
		// Komutlar '\r\n' ile bitiyorsa
		while ((endOfCommand = clientBuffer.find("\r\n")) != string::npos)
		{
			command = clientBuffer.substr(0, endOfCommand);
			clientBuffer.erase(0, endOfCommand + 2);  // '\r\n' karakterlerini sil
			// Komutu işleyen CommandParser sınıfı kullanılır.
			CommandParser::commandParser(command.c_str(), _clients[clientSocketFD], this);
		}
		clientBuffer.clear();
	}
}

// İstemci isteğini işler
// İstemci soket dosya tanımlayıcısını alır ve isteği işler.
void Server::handleClient(int clientSocketFD)
{
		const size_t BUFFER_SIZE = 512;
		char tempBuffer[BUFFER_SIZE];
		memset(tempBuffer, 0, BUFFER_SIZE);

		// İstemciden gelen veriyi okur.
		ssize_t received = recv(clientSocketFD, tempBuffer, BUFFER_SIZE - 1, 0);
		if (received > 0) {
			// Gelen veriyi istemci tamponuna ekler ve kısmi komutları işler.
			clientBuffers[clientSocketFD].appendtoBuffer(string(tempBuffer, received));
			cout << "Received: " << tempBuffer << endl;
			processPartialCommands(clientSocketFD);
		} else if (received == 0 || errno == ECONNRESET) {
			// İstemci bağlantısı kapatıldıysa veya hata durumunda
			FD_CLR(clientSocketFD, &read_set);
			clientDisconnect(clientSocketFD);
			clientBuffers.erase(clientSocketFD);
		} else {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				// Hata durumunda
				FD_CLR(clientSocketFD, &read_set);
				ErrorLogger("recv error", __FILE__, __LINE__, false);
				close(clientSocketFD);
				clientBuffers.erase(clientSocketFD);
			}
		}
	//}
}

// İstemci bağlantısını sonlandırır
// İstemci tüm kanallardan çıkarılır ve bağlantı kapatılır.
void Server::clientDisconnect(int clientSocketFD)
{
     try
    {
        // İstemci haritasında istemci aranır ve bulunursa işlemler gerçekleştirilir.
        std::map<int, Client*>::iterator it = _clients.find(clientSocketFD);
        if (it == _clients.end()) {
            error(RED, "Client not found in client map.\n", RESET);
            return;
        }

        // Tüm kanallardan istemci çıkarılır ve ayrılır.
        removeClientFromAllChannels(it->second);
        it->second->leave();

        // İstemci bağlantısı kapatıldı bilgisi loglanır.
        ostringstream messageStreamDisconnect;
        messageStreamDisconnect << "Client " << it->second->getNickName() << " has disconnected.";
        log(messageStreamDisconnect.str());

        FD_CLR(clientSocketFD, &read_set);

     // İstemci soketi kapatılır ve bellekten temizlenir.
        close(clientSocketFD);
        delete it->second;
        _clients.erase(it);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}

// Sunucu şifresini ayarlar
void Server::setSrvPass(const string& pass) {
	_serverPass = pass;
}

// Verilen şifreyi sunucu şifresiyle karşılaştırır
bool Server::verifySrvPass(const string& pass) {
	if (_serverPass == pass) {
		return true;
	}
	return false;
}
// Sinyali işler ve sunucuyu kapatır.
void Server::signalHandler(int signum)
{
	Server::getInstance()->shutdownSrv();
	exit(signum);
}
// Tüm kanalları kaldırır ve bağlantıyı kapatır.
void Server::shutdownSrv()
{
	std::cout << "Shutting down server..." << std::endl;

	// Tüm istemciler için işlemler gerçekleştirilir.
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		Client* client = it->second;
		if (client != NULL) {
			// İstemciye kapatılma mesajı gönderilir ve tüm kanallardan çıkarılır.
			client->sendMessage("ERROR :Closing Link: " + client->getHostName() + " (Server shutdown)");
			removeClientFromAllChannels(client);
			close(it->first);
			delete client;
		}
	}
	_clients.clear();

	// Sunucu soketi kapatılır.
	if (_serverSocketFD != -1) {
		close(_serverSocketFD);
		_serverSocketFD = -1;
	}

	// Bot nesnesi bellekten temizlenir.
	if (_bot != NULL) {
		delete _bot;
		_bot = NULL;
	}

	FD_ZERO(&read_set);

	// Bellekte sızıntı kontrolü yapılır.
	//system("leaks ircserv"); //control c yapınca leaks bilgisi görmek istiyorsanız bunu açın
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
