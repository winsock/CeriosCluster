//
//  PlayerPositionAndLookPacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 9/14/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef PlayerPositionAndLookPacket_hpp
#define PlayerPositionAndLookPacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class PlayerPositionAndLookPacket : public Packet {
        typedef union RelativeFlags {
            std::uint8_t rawFlags;
            BitField<0> x;
            BitField<1> y;
            BitField<2> z;
            BitField<3> yRotation;
            BitField<4> xRotation;
        } RelativeFlags;
    public:
        std::double_t x, y, z;
        std::float_t yaw, pitch;
        RelativeFlags flags;
    public:
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<PlayerPositionAndLookPacket>(new PlayerPositionAndLookPacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<PlayerPositionAndLookPacket>(new PlayerPositionAndLookPacket())); }
    protected:
        PlayerPositionAndLookPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        PlayerPositionAndLookPacket();
    };
}}
#endif /* PlayerPositionAndLookPacket_hpp */
