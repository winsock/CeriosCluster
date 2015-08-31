//
//  EncryptionPacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 8/31/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef EncryptionPacket_hpp
#define EncryptionPacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class EncryptionPacket : public Packet {
    public:
        std::string serverId;
        std::vector<std::int8_t> pubKeyData;
        std::vector<std::int8_t> verifyTokenData;

    public:
        void sendTo(Cerios::Server::AbstractClient *client);
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<EncryptionPacket>(new EncryptionPacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket() { return std::static_pointer_cast<Packet>(std::shared_ptr<EncryptionPacket>(new EncryptionPacket())); }
    protected:
        EncryptionPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        EncryptionPacket();
    };
}}
#pragma GCC visibility pop
#endif /* EncryptionPacket_hpp */
