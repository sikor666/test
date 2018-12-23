#pragma once

#include "IPacket.hpp"

namespace Protocol
{

class Paddle final : public IPacket
{
public:
    Paddle(PacketType type_, Buffer&& buffer_, Game::State& state_, SocketAddress&& client_) :
        type{ type_ },
        buffer{ std::move(buffer_) },
        state{ state_ },
        client{ std::move(client_) }
    {
    }

    virtual Buffer serialize() override
    {
        return type == PacketType::SetPaddlePosition ? set() : get();
    }

private:
    Buffer set()
    {
        Pack pack;
        PaddlePosition paddle;

        bufferRead(buffer, pack);
        bufferRead(buffer, paddle);

        state.paddles[paddle.index] = paddle.position;

        return {};
    }

    Buffer get()
    {
        Header header;
        header.type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::GetPaddlesPositions);

        Info info;
        info.role = static_cast<std::underlying_type<ClientType>::type>
            (state.getClientRole(client));

        Pack pack;
        pack.size = state.paddles.size();

        Buffer response;
        bufferInsert(response, header);
        bufferInsert(response, info);
        bufferInsert(response, pack);

        PaddlePosition paddle;

        for (const auto& p : state.paddles)
        {
            paddle.index = p.first;
            paddle.position = p.second;

            bufferInsert(response, paddle);
        }

        return response;
    }

private:
    PacketType type;
    Buffer buffer;
    Game::State& state;
    SocketAddress client;
};

} // namespace Protocol
