#pragma once

#include "IPacket.hpp"

namespace Protocol
{

class Live final : public IPacket
{
public:
    Live(PacketType type_, Buffer&& buffer_, Game::State& state_, SocketAddress&& client_) :
        type{ type_ },
        buffer{ std::move(buffer_) },
        state{ state_ },
        client{ std::move(client_) }
    {
    }

    virtual Buffer serialize() override
    {
        return type == PacketType::SetNumberLives ? set() : get();
    }

private:
    Buffer set()
    {
        Lives lives;
        bufferRead(buffer, lives);

        state.lives = lives.number;

        return {};
    }

    Buffer get()
    {
        Buffer response;

        Info info;
        info.role = static_cast<std::underlying_type<ClientType>::type>
            (state.getClientRole(client));
        bufferInsert(response, info);

        Lives lives;
        lives.number = state.lives;
        bufferInsert(response, lives);

        return response;
    }

private:
    PacketType type;
    Buffer buffer;
    Game::State& state;
    SocketAddress client;
};

} // namespace Protocol
