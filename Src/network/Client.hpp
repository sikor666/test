#pragma once

#include "udp/Socket.hpp"
#include "protocol/Protocol.hpp"

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

using namespace Protocol;

class Client
{
public:
    Client()
    {
        socket.connect(SERV_ADDR, SERV_PORT);
    }

    bool connect()
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>(PacketType::Connect);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        Buffer response = send(std::move(buffer));

        Info info;

        if (response.size() == sizeof(info))
        {
            bufferRead(response, info);

            role = static_cast<ClientType>(info.role);

            return true;
        }

        return false;
    }

    bool disconnect()
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>(PacketType::Disconnect);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        Buffer response = send(std::move(buffer));

        return true;
    }

    void setBallParameters(float ballSpeed, float ballDirX, float ballDirY,
                           float ballPosLeft, float ballPosTop)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::SetBallParameters);

        Header header;
        header.type = type;

        BallParameters ball;
        ball.speed = ballSpeed;
        ball.dirx = ballDirX;
        ball.diry = ballDirY;
        ball.left = ballPosLeft;
        ball.top = ballPosTop;

        Buffer buffer;
        bufferInsert(buffer, header);
        bufferInsert(buffer, ball);

        Buffer response = send(std::move(buffer));
    }

    template<typename T>
    void setBlocksParameters(T blocks)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::SetBlocksParameters);

        Header header;
        header.type = type;

        Pack pack;
        pack.size = blocks.size();

        Buffer buffer;
        bufferInsert(buffer, header);
        bufferInsert(buffer, pack);

        BlocksParameters block;

        for (const auto& b : blocks)
        {
            block.level = b.level;
            block.left = b.rect.left;
            block.top = b.rect.top;
            bufferInsert(buffer, block);
        }

        Buffer response = send(std::move(buffer));
    }

    void setPlayerPosition(int playerIndex, float playerPosition)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::SetPaddlePosition);

        Header header;
        header.type = type;

        Pack pack;
        pack.size = 1;

        PaddlePosition paddle;
        paddle.index = playerIndex;
        paddle.position = playerPosition;

        Buffer buffer;
        bufferInsert(buffer, header);
        bufferInsert(buffer, pack);
        bufferInsert(buffer, paddle);

        Buffer response = send(std::move(buffer));
    }

    void setNumberLives(int numberLives)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::SetNumberLives);

        Header header;
        header.type = type;

        Lives lives;
        lives.number = numberLives;

        Buffer buffer;
        bufferInsert(buffer, header);
        bufferInsert(buffer, lives);

        Buffer response = send(std::move(buffer));
    }

    template<typename T>
    void getPlayersPositions(T& players)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::GetPaddlesPositions);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        Buffer response = send(std::move(buffer));

        Info info;
        Pack pack;
        PaddlePosition paddle;

        if (response.size() == sizeof(info) + sizeof(pack) + sizeof(paddle) * 4)
        {
            bufferRead(response, info);
            role = static_cast<ClientType>(info.role);

            bufferRead(response, pack);

            for (size_t i = 0; i < pack.size; i++)
            {
                bufferRead(response, paddle);
                players[paddle.index].rect.left = paddle.position;
            }
        }
    }

    void getBallParameters(float& ballSpeed, float& ballDirX, float& ballDirY,
                           float& ballPosLeft, float& ballPosTop)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::GetBallParameters);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        Buffer response = send(std::move(buffer));

        Info info;
        BallParameters ball;

        if (response.size() == sizeof(info) + sizeof(ball))
        {
            bufferRead(response, info);
            role = static_cast<ClientType>(info.role);

            bufferRead(response, ball);

            ballSpeed = ball.speed;
            ballDirX = ball.dirx;
            ballDirY = ball.diry;
            ballPosLeft = ball.left;
            ballPosTop = ball.top;
        }
    }

    template<typename T, typename Block, typename Rect>
    void getBlocksParameters(float blockSizeX, float blockSizeY, T& blocks)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::GetBlocksParameters);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        Buffer response = send(std::move(buffer));

        Info info;
        bufferRead(response, info);
        role = static_cast<ClientType>(info.role);

        Pack pack;
        bufferRead(response, pack);

        blocks.clear();

        Block b;
        BlocksParameters block;

        //FIXME: Pack size sometimes is to long, when two servers running
        for (size_t i = 0; i < pack.size; i++)
        {
            bufferRead(response, block);
            b.rect = Rect(block.left, block.top, blockSizeX, blockSizeY);
            b.level = block.level;
            blocks.push_back(b);
        }
    }

    void getNumberLives(int& numberLives)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::GetNumberLives);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        Buffer response = send(std::move(buffer));

        Info info;
        Lives lives;

        if (response.size() == sizeof(info) + sizeof(lives))
        {
            bufferRead(response, info);
            role = static_cast<ClientType>(info.role);

            bufferRead(response, lives);
            numberLives = lives.number;
        }
    }

    bool isProvider()
    {
        return role == ClientType::Provider;
    }

private:
    Buffer send(Buffer&& message)
    {
        socket.send_to(message.data(), message.size());

        std::string endpoint;
        int n = socket.recv_from(buffer, endpoint);

        return Buffer(buffer, buffer + n);
    }

private:
    UDP::Socket socket;
    UDP::Buffer buffer;

    ClientType role = ClientType::Receiver;
};
