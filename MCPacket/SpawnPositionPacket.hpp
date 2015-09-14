//
//  SpawnPositionPacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 9/14/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef SpawnPositionPacket_hpp
#define SpawnPositionPacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class SpawnPositionPacket : public Packet {
    public:
        Position spawnPosition;
    public:
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<SpawnPositionPacket>(new SpawnPositionPacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<SpawnPositionPacket>(new SpawnPositionPacket())); }
    protected:
        SpawnPositionPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        SpawnPositionPacket();
    };
}}
#pragma GCC visibility pop
#endif /* SpawnPositionPacket_hpp */
