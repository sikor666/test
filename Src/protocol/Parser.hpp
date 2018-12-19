#pragma once

#include "game/State.hpp"
#include "udp/Socket.hpp"
#include "protocol/Connection.hpp"
#include "protocol/Paddle.hpp"
#include "protocol/Ball.hpp"
#include "protocol/Block.hpp"
#include "protocol/Live.hpp"
#include "protocol/Unknown.hpp"

#include <memory>

namespace Protocol
{

class Parser final
{
public:
    Parser() = default;
    ~Parser() = default;

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    Parser(const Parser&&) = delete;
    Parser& operator=(const Parser&&) = delete;

    std::unique_ptr<IPacket> createPacket(Buffer&& buffer,
                                          Game::State& state,
                                          SocketAddress&& client)
    {
        Header header;
        bufferRead(buffer, header);

        PacketType packetType;

        try
        {
            packetType = static_cast<PacketType>(header.type);
        }
        catch (const std::exception&)
        {
            packetType = PacketType::Unknown;
        }

        switch (packetType)
        {
        case PacketType::Connect:
        case PacketType::Disconnect:
            return std::make_unique<Connection>
                (packetType, std::move(buffer), state, std::move(client));

        case PacketType::SetPaddlePosition:
        case PacketType::GetPaddlesPositions:
            return std::make_unique<Paddle>
                (packetType, std::move(buffer), state, std::move(client));

        case PacketType::SetBallParameters:
        case PacketType::GetBallParameters:
            return std::make_unique<Ball>
                (packetType, std::move(buffer), state, std::move(client));

        case PacketType::SetBlocksParameters:
        case PacketType::GetBlocksParameters:
            return std::make_unique<Block>
                (packetType, std::move(buffer), state, std::move(client));

        case PacketType::SetNumberLives:
        case PacketType::GetNumberLives:
            return std::make_unique<Live>
                (packetType, std::move(buffer), state, std::move(client));
        }

        return std::make_unique<Unknown>();
    }
};

} // namespace Protocol
