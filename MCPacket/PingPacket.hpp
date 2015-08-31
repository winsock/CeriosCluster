//
//  PingPacket.hpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/30/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef PingPacket_hpp
#define PingPacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class PingPacket : public Packet {
    public:
        std::int64_t unixEpoch;
    public:
        void sendTo(Cerios::Server::AbstractClient *client);
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<PingPacket>(new PingPacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket() { return std::static_pointer_cast<Packet>(std::shared_ptr<PingPacket>(new PingPacket())); }
    protected:
        PingPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        PingPacket();
    };
}}
#pragma GCC visibility pop

#endif /* PingPacket_hpp */
