//
//  LoginSuccessPacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 9/2/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef LoginSuccessPacket_hpp
#define LoginSuccessPacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class LoginSuccessPacket : public Packet {
    public:
        std::string uuid, username;
    public:
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<LoginSuccessPacket>(new LoginSuccessPacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<LoginSuccessPacket>(new LoginSuccessPacket())); }
    protected:
        LoginSuccessPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        LoginSuccessPacket();
    };
}}
#pragma GCC visibility pop

#endif /* LoginSuccessPacket_hpp */
