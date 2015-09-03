//
//  JoinGamePacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 9/2/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef JoinGamePacket_hpp
#define JoinGamePacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    
    class JoinGamePacket : public Packet {
    public:
        std::int32_t playerEntityId;
        std::uint8_t gamemode;
        std::int8_t dimensionId;
        std::uint8_t difficulty;
        std::uint8_t maxPlayersOnPlayerList;
        std::string levelType;
        bool reducedDebugInfo;
    public:
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<JoinGamePacket>(new JoinGamePacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<JoinGamePacket>(new JoinGamePacket())); }
    protected:
        JoinGamePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        JoinGamePacket();
    };
}}
#pragma GCC visibility pop

#endif /* JoinGamePacket_hpp */
