#include "../include/Bot.hpp"

Bot::Bot(const string &serv, int port, const string &pass)
	: serv(serv),
	  port(port),
	  botPass(pass),
	  nick("GaribanBot"),
	  user("IRC"),
	  realname("Bot")
{
	connectServ();
}

Bot::~Bot()
{
	close(sock);
}

void Bot::connectServ()
{
	struct sockaddr_in server_addr;
	struct hostent *host;

	host = gethostbyname(serv.c_str());
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		error(RED, FAILED_SOCKET, RESET);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&(server_addr.sin_zero), '\0', 8);

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
		error(RED, FAILED_SOCKET_CONNECT, RESET);

	if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
		error(RED, FAILED_SOCKET_NONBLOCKING, RESET);

	if (sock != -1)
	{
		sendRegMsg("PASS " + botPass + "\r\n");
		sendRegMsg("NICK " + nick + "\r\n");
		sendRegMsg("USER " + user + " 0 * :" + realname + "\r\n");
	}
}

void Bot::listen()
{
	const int BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	string accumulated;

	int bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
	if (bytes_read < 0)
	{
		string errrecv = "Error in recv: " + string(strerror(errno)) + "\n";
		error(RED, errrecv.c_str(), RESET);
	}
	else if (bytes_read == 0)
		error(RED, "Connection closed by peer.\n", RESET);
	buffer[bytes_read] = '\0';
	accumulated += buffer;

	size_t pos;
	while ((pos = accumulated.find("\r\n")) != string::npos)
	{
		string msg = accumulated.substr(0, pos);
		accumulated.erase(0, pos + 2);
		processMessage(msg);
	}
}

void Bot::processMessage(const string &msg)
{
	if (msg.find("PRIVMSG") != string::npos)
	{
		string senderNick = msg.substr(1, msg.find("!") - 1);
		
		if (msg.find("hello") != string::npos)
			sendMsg(senderNick, "Hello " + senderNick + "!");

		if (msg.find("time") != string::npos || msg.find("date") != string::npos)
		{
			time_t rawtime;
			struct tm *timeinfo;
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			sendMsg(senderNick, asctime(timeinfo));
		}

		if (msg.find("who are you") != string::npos)
			sendMsg(senderNick, "I am a bot, I am here to help you.");
		
		if (msg.find("what is the meaning of life") != string::npos)
			sendMsg(senderNick, "42");

		if (msg.find("amk") != string::npos)
			sendMsg(senderNick, "Saygılı ol lütfen  " + senderNick + "!");
	}
}
void Bot::sendMsg(const string &channel, const string &message)
{
	string fullMessage = "PRIVMSG " + channel + " :" + message + "\r\n";
	send(sock, fullMessage.c_str(), fullMessage.length(), 0);
}

void Bot::sendRegMsg(const string &message)
{
	send(sock, message.c_str(), message.length(), 0);
}
