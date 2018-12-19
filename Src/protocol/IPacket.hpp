#pragma once

#include "Protocol.hpp"

namespace Protocol
{

class IPacket
{
public:
    virtual Buffer serialize() = 0;

    ~IPacket() = default;
};

} // namespace Protocol
