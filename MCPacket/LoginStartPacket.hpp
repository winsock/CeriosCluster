//
//  LoginStartPacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 8/31/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef LoginStartPacket_hpp
#define LoginStartPacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class LoginStartPacket : public Packet {
    public:
        std::string serverName;
    public:
        void sendTo(Cerios::Server::AbstractClient *client);
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<LoginStartPacket>(new LoginStartPacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket() { return std::static_pointer_cast<Packet>(std::shared_ptr<LoginStartPacket>(new LoginStartPacket())); }
    protected:
        LoginStartPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        LoginStartPacket() : Packet(0x00) { }
    };
}}
#pragma GCC visibility pop

#endif /* LoginStartPacket_hpp */
