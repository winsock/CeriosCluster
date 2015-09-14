//
//  PlayerAbilitiesPacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 9/14/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef PlayerAbilitiesPacket_hpp
#define PlayerAbilitiesPacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class PlayerAbilitiesPacket : public Packet {
        typedef union AbilityFlags {
            std::uint8_t rawFlags;
            BitField<0> isCreative;
            BitField<1> isFlying;
            BitField<2> canFly;
            BitField<3> godMode;
        } AbilityFlags;
    public:
        AbilityFlags abilities;
        std::float_t flyingSpeed, walkingSpeed;
    public:
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<PlayerAbilitiesPacket>(new PlayerAbilitiesPacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<PlayerAbilitiesPacket>(new PlayerAbilitiesPacket())); }
    protected:
        PlayerAbilitiesPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        PlayerAbilitiesPacket();
    };
}}
#pragma GCC visibility pop
#endif /* PlayerAbilitiesPacket_hpp */
