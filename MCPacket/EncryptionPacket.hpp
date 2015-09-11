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

#include <array>

#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class EncryptionPacket : public Packet {
    public:
        std::string serverId, publickKey;
        std::array<std::uint8_t, 16> clearVerifyToken;
        std::vector<std::uint8_t> clearSharedSecret;
        std::vector<std::uint8_t> sealedVerifyToken;
        std::vector<std::uint8_t> sealedSharedSecret;
    public:
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<EncryptionPacket>(new EncryptionPacket(side, packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<EncryptionPacket>(new EncryptionPacket(side))); }
    protected:
        EncryptionPacket(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        EncryptionPacket(Cerios::Server::Side side);
    };
}}
#pragma GCC visibility pop
#endif /* EncryptionPacket_hpp */
