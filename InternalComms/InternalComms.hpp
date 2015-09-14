/*
 *  InternalComms.hpp
 *  InternalComms
 *
 *  Created by Andrew Querol on 9/7/15.
 *  Copyright © 2015 Andrew Querol. All rights reserved.
 *
 */

#ifndef InternalComms_
#define InternalComms_

#include <vector>
#include <cstdint>
#include <memory>

/* The classes below are exported */
#pragma GCC visibility push(default)

namespace Cerios { namespace InternalComms {
    enum class MessageID : std::uint8_t {
        ACK,
        NAK,
        ACCEPT_CLIENT,
        MC_PACKET
    };
    
    class Packet : public std::enable_shared_from_this<Packet>  {
    public:
        virtual void serializeData(std::vector<std::uint8_t> &outputBuffer) = 0;
        virtual MessageID getMessageID() = 0;
        virtual std::weak_ptr<std::vector<std::uint8_t>> getPayload() = 0;
        virtual void setPacketNumber(std::uint8_t packetNumber) = 0;
        virtual std::uint8_t getPacketNumber() = 0;
        virtual std::string getPlayerID() = 0;

        static std::shared_ptr<Packet> newPacket(MessageID messageType);
        static std::shared_ptr<Packet> newPacket(MessageID messageType, std::vector<std::uint8_t> &payload);
        static std::shared_ptr<Packet> newPacket(MessageID messageType, std::string playerId);
        static std::shared_ptr<Packet> newPacket(MessageID messageType, std::string playerId, std::vector<std::uint8_t> &payload);

        static std::shared_ptr<Packet> fromData(std::vector<std::uint8_t> &rawData, bool consume = false);
    protected:
        Packet() = default;
    };
}}

#pragma GCC visibility pop
#endif
