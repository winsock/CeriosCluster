    //
//  ServerHandshakePacket.hpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/29/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef HandshakePacket_hpp
#define HandshakePacket_hpp

#include "Packet.hpp"

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class HandshakePacket : public Packet {
    public:
        std::int32_t protocolVersion;
        std::string serverAddress;
        std::uint16_t serverPort;
        ClientState requestedNextState;
    public:
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<HandshakePacket>(new HandshakePacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<HandshakePacket>(new HandshakePacket())); }
    protected:
        HandshakePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        HandshakePacket();
    };
}}
#pragma GCC visibility pop

#endif /* ServerHandshakePacket_hpp */
