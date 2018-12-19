#pragma once

#include "IPacket.hpp"

namespace Protocol
{

class Block final : public IPacket
{
public:
    Block(PacketType type_, Buffer&& buffer_, Game::State& state_, SocketAddress&& client_) :
        type{ type_ },
        buffer{ std::move(buffer_) },
        state{ state_ },
        client{ std::move(client_) }
    {
    }

    virtual Buffer serialize() override
    {
        return type == PacketType::SetBlocksParameters ? set() : get();
    }

private:
    Buffer set()
    {
        state.blocks.clear();

        Pack pack;
        bufferRead(buffer, pack);

        BlocksParameters block;

        for (int i = 0; i < pack.size; i++)
        {
            bufferRead(buffer, block);
            state.blocks.push_back(block);
        }

        return {};
    }

    Buffer get()
    {
        Buffer response;

        Info info;
        info.role = static_cast<std::underlying_type<ClientType>::type>(state.getClientRole(client));

        Pack pack;
        pack.size = state.blocks.size();

        bufferInsert(response, info);
        bufferInsert(response, pack);

        for (auto b : state.blocks)
        {
            bufferInsert(response, b);
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
