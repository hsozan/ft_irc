#include "../include/Server.hpp"

void Server::removeChannel(const std::string& channelName)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(channelName);

	if (it != _channels.end())
	{
		Channel* channel = it->second;
		if (channel != NULL)
		{
			delete channel;
			_channels.erase(it);
		}
		else
			std::cout << "Channel " + channelName + " is already null." << std::endl;
	}
	else
		std::cout << "Channel " + channelName + " does not exist."<< std::endl;
}

void Server::removeClientFromAllChannels(Client* client)
{
	if (client == NULL || !client->isSocketOpen())
		return;

	std::vector<Channel*> channels = client->getChannels();
	while (!channels.empty())
	{
		Channel* channel = channels.back();
		channels.pop_back();

		if (channel != NULL)
		{
			string channelName = channel->getChannelName();
			string clientNick = client->getNickName();
			channel->removeUserFromChannel(client);
			client->removeChannel(channel);

			string leaveMessage = clientNick + " has left the channel " + channelName;
			log(leaveMessage);

			if (channel->getChannelClientCount() == 0 && channelExists(channel->getChannelName()))
			{
				std::cout << "Channel " + channelName + " is empty, deleting."<< std::endl;
				removeChannel(channelName);
			}
		}
	}
}

void Server::addChannel(Channel* channel)
{
	_channels.insert(std::make_pair(channel->getChannelName(), channel));
}

bool Server::channelExists(const string& channelName)
{
	for (map<string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->second->getChannelName() == channelName)
			return true;
	}
	return false;
}

Client* Server::getClient(string& nickName)
{
	for (map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickName() == nickName)
			return it->second;
	}
	return NULL;
}

Channel* Server::getChannels(string& channelName)
{
	for (map<string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->second->getChannelName() == channelName)
			return it->second;
	}
	return NULL;
}
