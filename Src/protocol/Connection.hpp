#pragma once

#include "IPacket.hpp"

namespace Protocol
{

class Connection final : public IPacket
{
public:
    Connection(PacketType type_,
        Buffer&& buffer_,
        Game::State& state_,
        SocketAddress&& client_) :
            type{ type_ },
            buffer{ std::move(buffer_) },
            state{ state_ },
            client{ std::move(client_) }
    {
    }

    virtual Buffer serialize() override
    {
        return type == PacketType::Connect ? connect() : disconnect();
    }

private:
    Buffer connect()
    {
        auto role = state.addClient(client);

        Info info;
        info.role = static_cast<std::underlying_type<ClientType>::type>(role);

        Buffer response;
        bufferInsert(response, info);

        return response;
    }

    Buffer disconnect()
    {
        state.removeClient(client);

        return {};
    }

    PacketType type;
    Buffer buffer;
    Game::State& state;
    SocketAddress client;
};

} // namespace Protocol
