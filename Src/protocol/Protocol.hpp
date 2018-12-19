#pragma once

#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace Protocol
{

enum class PacketType : int
{
    Unknown = 0,
    Connect,
    Disconnect,
    SetPaddlePosition,
    GetPaddlesPositions,
    SetBallParameters,
    GetBallParameters,
    SetBlocksParameters,
    GetBlocksParameters,
    SetNumberLives,
    GetNumberLives,
};

enum class ClientType : int
{
    Unknown = 0,
    Provider,
    Receiver,
};

struct ClientInfo
{
    ClientType type = ClientType::Unknown;
    std::time_t time = 0;
};

using SocketAddress = std::string;
using Byte = char;
using Buffer = std::vector<Byte>;

#pragma pack (push, 1)
struct Header
{
    int type;
};

struct Pack
{
    int size;
};

struct Info
{
    int role;
};

struct PaddlePosition
{
    int index;
    float position;
};

struct BallParameters
{
    float speed;
    float dirx;
    float diry;
    float left;
    float top;
};

struct BlocksParameters
{
    int level;
    float left;
    float top;
};

struct Lives
{
    int number;
};
#pragma pack (pop)

template<typename B, typename D>
void bufferInsert(B& buffer, D& data)
{
    buffer.insert(buffer.begin(),
        reinterpret_cast<Byte *>(&data),
        reinterpret_cast<Byte *>(&data) + sizeof(data));
}

template<typename B, typename D>
void bufferRead(B& buffer, D& data)
{
    memcpy(reinterpret_cast<Byte *>(&data),
        buffer.data() + buffer.size() - sizeof(data),
        sizeof(data));

    buffer.resize(buffer.size() - sizeof(data));
}

} // namespace Protocol
