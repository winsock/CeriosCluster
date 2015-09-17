//
//  ChatMessagePacket.hpp
//  MCPacket
//
//  Created by Andrew Querol on 9/16/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef ChatMessagePacket_hpp
#define ChatMessagePacket_hpp

#include "Packet.hpp"

#define MC_CHAT_MAX_TEXT_LENGTH 100

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class ChatMessagePacket : public Packet {
    public:
        enum class ChatType : std::uint8_t {
            NORMAL= 0,
            SYSTEM = 1,
            ABOVE_HOTBAR = 2
        };
        
        std::string jsonChatData;
        ChatType chatPosition;
    public:
        void serializePacket(Cerios::Server::Side sideSending);
        
        static std::shared_ptr<Packet> parsePacket(Cerios::Server::Side side, std::shared_ptr<Packet> packetInProgress) { return std::static_pointer_cast<Packet>(std::shared_ptr<ChatMessagePacket>(new ChatMessagePacket(side, packetInProgress))); }
        static std::shared_ptr<Packet> newPacket(Cerios::Server::Side side) { return std::static_pointer_cast<Packet>(std::shared_ptr<ChatMessagePacket>(new ChatMessagePacket(side))); }
    protected:
        ChatMessagePacket(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        ChatMessagePacket(Cerios::Server::Side side);
    };
}}
#pragma GCC visibility pop
#endif /* ChatMessagePacket_hpp */
