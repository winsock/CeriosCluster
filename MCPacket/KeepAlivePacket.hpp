//
//  KeepAlivePacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 9/13/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef KeepAlivePacket_hpp
#define KeepAlivePacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class KeepAlivePacket : public Packet {
    public:
        std::int32_t keepAliveId;
    public:
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<KeepAlivePacket>(new KeepAlivePacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<KeepAlivePacket>(new KeepAlivePacket())); }
    protected:
        KeepAlivePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        KeepAlivePacket();
    };
}}
#pragma GCC visibility pop

#endif /* KeepAlivePacket_hpp */
