#pragma once

#include "protocol/Protocol.hpp"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <map>
#include <unordered_map>

namespace Game
{

using namespace Protocol;

class State final
{
public:
    State() = default;
    ~State() = default;

    State(const State&) = delete;
    State& operator=(const State&) = delete;

    State(const State&&) = delete;
    State& operator=(const State&&) = delete;

    ClientType addClient(const SocketAddress& client)
    {
        bool foundProvider = std::find_if(std::begin(clients), std::end(clients),
            [](const std::pair<SocketAddress, ClientInfo>& c)
        {
            return c.second.type == ClientType::Provider;

        }) != std::end(clients);

        ClientInfo info;
        info.type = foundProvider ? ClientType::Receiver : ClientType::Provider;
        info.time = std::time(nullptr);

        clients[client] = info;

        std::cout << "Connect client: " << client << std::endl;

        return info.type;
    }

    void removeClient(const SocketAddress& client)
    {
        clients.erase(client);

        std::cout << "Disconnect client: " << client << std::endl;
    }

    ClientType getClientRole(const SocketAddress& client)
    {
        if (clients[client].type == ClientType::Provider)
        {
            return ClientType::Provider;
        }

        bool foundProvider = std::find_if(std::begin(clients), std::end(clients),
            [](const std::pair<SocketAddress, ClientInfo>& c)
        {
            return c.second.type == ClientType::Provider;

        }) != std::end(clients);

        if (!foundProvider)
        {
            setProvider(client);

            return ClientType::Provider;
        }

        return ClientType::Receiver;
    }

    void updateClientActivity(const SocketAddress& client)
    {
        clients[client].time = std::time(nullptr);
    }

    void removeInactiveClients()
    {
        for (auto it = clients.cbegin(); it != clients.cend(); /* no increment */)
        {
            std::time_t delay = (std::time(nullptr) - (*it).second.time);

            if (delay > 3)
            {
                std::cout << "Remove inactive client: " << (*it).first << std::endl;

                it = clients.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

public:
    std::unordered_map<SocketAddress, ClientInfo> clients;
    std::map<int, float> paddles;
    std::vector<BlocksParameters> blocks;
    BallParameters ball;
    int lives;

private:
    void setProvider(const SocketAddress& client)
    {
        for (auto& c : clients)
        {
            if (c.first == client)
            {
                c.second.type = ClientType::Provider;
                return;
            }
        }
    }
};

} // namespace Game
