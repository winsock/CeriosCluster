/*
 *  InternalCommsPriv.hpp
 *  InternalComms
 *
 *  Created by Andrew Querol on 9/7/15.
 *  Copyright © 2015 Andrew Querol. All rights reserved.
 *
 */

#include "InternalComms.hpp"

/* The classes below are not exported */
#pragma GCC visibility push(hidden)

namespace Cerios { namespace InternalComms {
#pragma pack(1)
    typedef struct {
        std::uint8_t id;
        std::uint8_t packetNumber;
        char playerUUID[36];
        std::size_t payloadLength;
    } MessagePacketHeader;
#pragma pack()
    
    class PacketImpl : public Packet {
    private:
        MessagePacketHeader packetHeader;
        std::shared_ptr<std::vector<std::uint8_t>> payload;
    public:
        PacketImpl(Cerios::InternalComms::MessageID id);
        PacketImpl(Cerios::InternalComms::MessageID id, std::vector<std::uint8_t> &payload);
        PacketImpl(Cerios::InternalComms::MessageID id, std::string playerId, std::vector<std::uint8_t> &payload);

        void serializeData(std::vector<std::uint8_t> &outputBuffer);
        MessageID getMessageID();
        std::weak_ptr<std::vector<std::uint8_t>> getPayload();
        void setPacketNumber(std::uint8_t packetNumber);
        std::uint8_t getPacketNumber();
        std::string getPlayerID();
    };
}}
#pragma GCC visibility pop
