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

#include <memory>
#include <string>

namespace Cerios { namespace Server {
    class HandshakePacket : public Packet {
    public:
        std::int32_t protocolVersion;
        std::string serverAddress;
        std::uint16_t serverPort;
        ClientState requestedNextState;
    public:
        void onReceivedBy(Cerios::Server::Client *client);
        static std::shared_ptr<Packet> parsePacket(std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<HandshakePacket>(new HandshakePacket(packetInProgress))); }
        static std::shared_ptr<Packet> newPacket() { return std::static_pointer_cast<Packet>(std::shared_ptr<HandshakePacket>(new HandshakePacket())); }
    protected:
        HandshakePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        HandshakePacket();
    };
}}

#endif /* ServerHandshakePacket_hpp */
