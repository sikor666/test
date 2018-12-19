#pragma once

#include "IPacket.hpp"

namespace Protocol
{

class Unknown final : public IPacket
{
public:
    virtual Buffer serialize() override
    {
        return {};
    }
};

} // namespace Protocol
