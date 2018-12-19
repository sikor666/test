#pragma once

#include "game/State.hpp"
#include "protocol/Parser.hpp"
#include "udp/Socket.hpp"

class Server
{
public:
    Server()
    {
        socket.bind(SERV_ADDR, SERV_PORT);
    }

    void run()
    {
        while (true)
        {
            Protocol::SocketAddress client;
            int n = socket.recv_from(buffer, client);

            state.updateClientActivity(client);

            auto packet = parser.createPacket
                (Protocol::Buffer(buffer, buffer + n), state, std::move(client));

            auto buffer = packet->serialize();

            socket.send_to(buffer.data(), buffer.size());

            state.removeInactiveClients();
        }
    }

private:
    UDP::Socket socket;
    UDP::Buffer buffer;

    Game::State state;
    Protocol::Parser parser;
};
