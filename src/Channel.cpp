#include "../include/Channel.hpp"

Channel::Channel(const string& channelName, const string& channelKey, Client* channelOwner)
	: _channelName(channelName),
	  _channelOwner(channelOwner),
	  _channelKey(channelKey),
	  _userLimit(1000),
	  _noExternalMessages(false),
	  _moderated(false)
{
}

Channel::~Channel()
{
}

void Channel::broadcastMessage(const string& message) const
{
	string tmp = message;
	for (vector<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		(*it)->sendMessage(message);
	}
}

void Channel::broadcastMessage(const string& message, Client* exceptClient) const
{
	for (vector<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (*it != exceptClient)
			(*it)->sendMessage(message);
	}
}

void Channel::addClient(Client* client)
{
	_clients.push_back(client);
}

void Channel::removeUserFromChannel(Client* client)
{
	for (vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (*it == client)
		{
			_clients.erase(it);
			break;
		}
	}

	if (client->isOperator() == true)
	{
		client->setOperator(false);
		for (vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if ((*it)->isOperator() == true)
				return;
		}

		if (_clients.size() > 0)
		{
			Client* newOperator = _clients.front();
			if (newOperator->isOperator() == false)
			{
				newOperator->setOperator(true);
				setChannelOwner(newOperator);
				broadcastMessage("MODE " + _channelName + " +o " + newOperator->getNickName());
			}
		}
	}
}

bool Channel::isUserOnChannel(Client* client) const
{
	for (vector<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (*it == client)
			return true;
	}
	return false;
}

void Channel::setUpModeChannel(Channel* channel, Client* client, string& mode, string& modeParams)
{
	if (!channel || !client)
	{
		error(RED, "Channel or Client is null, can not set mode.", RESET);
	}

	if (mode == "+k" && client->isOperator())
	{
		channel->setChannelKey(modeParams);
		channel->broadcastMessage("MODE " + channel->getChannelName() + " +k " + client->getNickName());
	}
	else if (mode == "+l" && client->isOperator())
	{
		if (atoi(modeParams.c_str()) < 0 || modeParams.empty() || modeParams.find_first_not_of("0123456789") != string::npos)
		{
			client->sendMessage(":" + client->getHostName() + " 477 " + client->getNickName() + " " +"Can't set channel limit to " + modeParams + "\r\n");
			return;
		}
		channel->setUserLimit(atoi(modeParams.c_str())); 
		channel->broadcastMessage("MODE " + channel->getChannelName() + " +l " + modeParams + " " + client->getNickName());
	}
	else if (mode == "+o" && client->isOperator())
	{
		Client* targetClient = Server::getInstance()->getClient(modeParams);
		if (!targetClient)
		{
			client->sendMessage(":" + client->getHostName() + " 401 " + client->getNickName() + " " + modeParams + " :No such nick/channel\r\n");
			return;
		}
		targetClient->setOperator(true);
		channel->broadcastMessage("MODE " + channel->getChannelName() + " +o " + targetClient->getNickName());
	}
	else if (mode == "+m" && client->isOperator())
	{
		channel->setModerated(true);
		channel->broadcastMessage("MODE " + channel->getChannelName() + " +m " + client->getNickName());
	}
	else if (mode == "+n" && client->isOperator())
	{
		channel->setNoExternalMessages(true);
		channel->broadcastMessage("MODE " + channel->getChannelName() + " +n " + client->getNickName());
	}
	else
	{
		client->sendMessage(":" + client->getHostName() + " 501 " + client->getNickName() + " :You're not channel operator\r\n");
	}
}

void Channel::setLowModeChannel(Channel* channel, Client* client, string& mode, string& modeParams)
{
	if (!channel || !client)
	{
		error(RED, "Channel or Client is null, cannot set mode.", RESET);
	}

	if (mode == "-k" && client->isOperator())
	{
		channel->setChannelKey("");
		channel->broadcastMessage("MODE " + channel->getChannelName() + " -k " + client->getNickName());
	}
	else if (mode == "-l" && client->isOperator())
	{
		channel->setUserLimit(1000);
		channel->broadcastMessage("MODE " + channel->getChannelName() + " -l " + client->getNickName());
	}
	else if (mode == "-n" && client->isOperator())
	{
		channel->setNoExternalMessages(false);
		channel->broadcastMessage("MODE " + channel->getChannelName() + " -n " + client->getNickName());
	}
	else if (mode == "-o" && client->isOperator())
	{
		Client* targetClient = Server::getInstance()->getClient(modeParams);
		if (!targetClient)
		{
			client->sendMessage(":" + client->getHostName() + " 401 " + client->getNickName() + " " + modeParams + " :No such nick/channel\r\n");
			return;
		}
		if (targetClient->getNickName() != channel->getChannelOwner()->getNickName())
		{
			targetClient->setOperator(false);
			channel->broadcastMessage("MODE " + channel->getChannelName() + " -o " + targetClient->getNickName());
		}
	}
	else if (mode == "-m" && client->isOperator())
	{
		channel->setModerated(false);
		channel->broadcastMessage("MODE " + channel->getChannelName() + " -m " + client->getNickName());
	}
	else
	{
		client->sendMessage(":" + client->getHostName() + " 501 " + client->getNickName() + " :You're not channel operator");
	}
}

std::vector<std::string> Channel::getChannelClientNickNames() const
{
	vector<string> nickNames;
	for (vector<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
		nickNames.push_back((*it)->getNickName());
	return nickNames;
}

string Channel::getExistingUsersNickList() const
{
	string nickList;
	for (std::vector<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it != _clients.begin())
			nickList += " ";
		nickList += (*it)->getNickName();
	}
	return nickList;
}
