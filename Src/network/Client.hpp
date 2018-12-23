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
        socket.connect(STUN_ADDR, STUN_PORT);
        //socket.unblock();
    }

    bool connect()
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::Connect);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        send(std::move(buffer));

        return true;
    }

    bool disconnect()
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::Disconnect);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        //send(std::move(buffer));

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

        //send(std::move(buffer));
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

        //send(std::move(buffer));
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

        //send(std::move(buffer));
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

        //send(std::move(buffer));
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

        //send(std::move(buffer));
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

        //send(std::move(buffer));
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

        //send(std::move(buffer));
    }

    void getNumberLives(int& numberLives)
    {
        auto type = static_cast<std::underlying_type<PacketType>::type>
            (PacketType::GetNumberLives);

        Header header;
        header.type = type;

        Buffer buffer;
        bufferInsert(buffer, header);

        //send(std::move(buffer));
    }

    template<typename R, typename B, typename P, typename T, typename V>
    void parseAll(R& ballRect, B& blockIn, P& players, T& blocks, V& ballDir, V blockSize,
                  float& ballSpeed, int& numberLives)
    {
        Buffer response;

        do
        {

        response = read();

        Header header;
        bufferRead(response, header);

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
        {
            Info info;

            if (response.size() == sizeof(info))
            {
                bufferRead(response, info);

                role = static_cast<ClientType>(info.role);
            }
            break;
        }
        case PacketType::GetPaddlesPositions:
        {
            Info info;
            Pack pack;
            PaddlePosition paddle;

            if (response.size() ==
                sizeof(info) +
                sizeof(pack) +
                sizeof(paddle) * 4)
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
            break;
        }
        case PacketType::GetBallParameters:
        {
            Info info;
            BallParameters ball;

            if (response.size() == sizeof(info) + sizeof(ball))
            {
                bufferRead(response, info);
                role = static_cast<ClientType>(info.role);

                bufferRead(response, ball);

                ballSpeed = ball.speed;
                ballDir.x = ball.dirx;
                ballDir.y = ball.diry;
                ballRect.left = ball.left;
                ballRect.top = ball.top;
            }
            break;
        }
        case PacketType::GetBlocksParameters:
        {
            Info info;
            Pack pack;
            BlocksParameters block;

            size_t ii = sizeof(info);
            size_t pp = sizeof(pack);
            size_t bb = sizeof(block);

            if (response.size() > sizeof(info) + sizeof(pack))
            {
                bufferRead(response, info);
                role = static_cast<ClientType>(info.role);

                bufferRead(response, pack);

                if (pack.size > 24) return;

                blocks.clear();

                //FIXME: Pack size sometimes is to long, when two servers running
                for (size_t i = 0; i < pack.size; i++)
                {
                    bufferRead(response, block);
                    blockIn.rect = Rect(block.left, block.top, blockSize.x, blockSize.y);
                    blockIn.level = block.level;
                    blocks.push_back(blockIn);
                }
            }
            break;
        }
        case PacketType::GetNumberLives:
        {
            Info info;
            Lives lives;

            if (response.size() == sizeof(info) + sizeof(lives))
            {
                bufferRead(response, info);
                role = static_cast<ClientType>(info.role);

                bufferRead(response, lives);
                numberLives = lives.number;
            }
            break;
        }
        }

        } while (response.size());
    }

    bool isProvider()
    {
        return role == ClientType::Provider;
    }

private:
    void send(Buffer&& message)
    {
        socket.send_to(message.data(), message.size());
    }

    Buffer read()
    {
        std::string endpoint;
        int n = socket.ready() ? socket.recv_from(buffer, endpoint) : 0;

        return Buffer(buffer, buffer + n);
    }

    /*void parse(Buffer&& message)
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

        case PacketType::GetPaddlesPositions:

        case PacketType::GetBallParameters:

        case PacketType::GetBlocksParameters:

        case PacketType::GetNumberLives:
        }
    }*/

private:
    UDP::Socket socket;
    UDP::Buffer buffer;

    ClientType role = ClientType::Receiver;
};
