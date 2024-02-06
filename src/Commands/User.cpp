#include "../../include/Commands.hpp"

void User::user(Client *client, vector<string> commandParts, Server *srv)
{
    if (!client || !srv)
    error(RED, "Error: client or srv is null\n", RESET);

    if (client->isRegistered())
    {
        client->sendReply(ERR_ALREADYREGISTERED(client->getNickName()));
        return;
    }

    if (commandParts.size() < 3)
    {
        client->sendReply(ERR_NEEDMOREPARAMS(client->getNickName(), "USER"));
        client->sendMessage("Usage: /USER <username> <realname>");
        return;
    }

    if (!client->getValidName(commandParts.at(1)) || !client->getValidName(commandParts.at(2)))
    {
        client->sendReply(ERR_ERRONEUSNICKNAME(client->getNickName(), commandParts.at(1)));
        return;
    }

    std::string userName = commandParts.at(1);
    std::string realName = commandParts.at(2);

    const std::map<int, Client *> &clients = srv->getAllClients();
    for (std::map<int, Client *>::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
        const Client *regUser = it->second;
        if (!regUser)
            continue;
        if (regUser != client && regUser->getUserName() == userName)
        {
            client->sendMessage("This username is already in use");
            return;
        }
    }

    client->setUserName(userName);
    client->setRealName(realName);
    client->setStatus(CLIENT_CONNECTED);
    client->setUserAuth(true);

    if (client->getUserAuth() == true)
    {
        Bot *bot = srv->getBot();
        if (bot)
            bot->WelcomeMsg(client->getNickName());
    }
}
