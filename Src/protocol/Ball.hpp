#pragma once

#include "IPacket.hpp"

namespace Protocol
{

class Ball final : public IPacket
{
public:
    Ball(PacketType type_, Buffer&& buffer_, Game::State& state_, SocketAddress&& client_) :
        type{ type_ },
        buffer{ std::move(buffer_) },
        state{ state_ },
        client{ std::move(client_) }
    {
    }

    virtual Buffer serialize() override
    {
        return type == PacketType::SetBallParameters ? set() : get();
    }

private:
    Buffer set()
    {
        BallParameters ball;
        bufferRead(buffer, ball);

        state.ball = ball;

        return {};
    }

    Buffer get()
    {
        Buffer response;

        Info info;
        info.role = static_cast<std::underlying_type<ClientType>::type>
            (state.getClientRole(client));

        bufferInsert(response, info);
        bufferInsert(response, state.ball);

        return response;
    }

private:
    PacketType type;
    Buffer buffer;
    Game::State& state;
    SocketAddress client;
};

} // namespace Protocol
