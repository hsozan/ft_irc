#include "../../include/Commands.hpp"

void Quit::quit(Client *client, vector<string> commandParts, Server *srv)
{
    (void)commandParts;
    client->sendMessage(RPL_QUIT(client->getPrefix(), "Leaving server..."));
    srv->removeClientFromAllChannels(client);
    srv->clientDisconnect(client->getClientSocketFD());
}
