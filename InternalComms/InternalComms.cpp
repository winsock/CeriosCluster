/*
 *  InternalComms.cpp
 *  InternalComms
 *
 *  Created by Andrew Querol on 9/7/15.
 *  Copyright © 2015 Andrew Querol. All rights reserved.
 *
 */

#include <iostream>
#include "InternalComms.hpp"
#include "InternalCommsPriv.hpp"

std::shared_ptr<Cerios::InternalComms::Packet> Cerios::InternalComms::Packet::newPacket(Cerios::InternalComms::MessageID messageType) {
    return std::dynamic_pointer_cast<Cerios::InternalComms::Packet>(std::shared_ptr<Cerios::InternalComms::PacketImpl>(new Cerios::InternalComms::PacketImpl(messageType)));
}

std::shared_ptr<Cerios::InternalComms::Packet> Cerios::InternalComms::Packet::newPacket(Cerios::InternalComms::MessageID messageType, std::vector<std::uint8_t> &payload) {
    return std::dynamic_pointer_cast<Cerios::InternalComms::Packet>(std::shared_ptr<Cerios::InternalComms::PacketImpl>(new Cerios::InternalComms::PacketImpl(messageType, payload)));
}

std::shared_ptr<Cerios::InternalComms::Packet> Cerios::InternalComms::Packet::fromData(std::vector<std::uint8_t> &rawData, bool consume) {
    std::shared_ptr<MessagePacketHeader> messageHeader(new MessagePacketHeader);
    std::memcpy(messageHeader.get(), rawData.data(), sizeof(MessagePacketHeader));
    if (rawData.size() < sizeof(MessagePacketHeader) + messageHeader->payloadLength) {
        return nullptr;
    }
    
    std::vector<std::uint8_t> payload(messageHeader->payloadLength);
    std::memcpy(payload.data(), rawData.data() + sizeof(MessagePacketHeader), messageHeader->payloadLength);
    if (consume) {
        rawData.erase(rawData.begin(), rawData.begin() + sizeof(MessagePacketHeader) + messageHeader->payloadLength);
    }
    
    return std::dynamic_pointer_cast<Cerios::InternalComms::Packet>(std::shared_ptr<Cerios::InternalComms::PacketImpl>(new Cerios::InternalComms::PacketImpl(static_cast<MessageID>(messageHeader->id), payload)));
}

Cerios::InternalComms::PacketImpl::PacketImpl(Cerios::InternalComms::MessageID id) : packetHeader(new MessagePacketHeader), payload(nullptr) {
    packetHeader->id = static_cast<std::uint8_t>(id);
    packetHeader->payloadLength = 0;
}

Cerios::InternalComms::PacketImpl::PacketImpl(Cerios::InternalComms::MessageID id, std::vector<std::uint8_t> &payload) : packetHeader(new MessagePacketHeader), payload(new std::vector<std::uint8_t>(payload)) {
    packetHeader->id = static_cast<std::uint8_t>(id);
    packetHeader->payloadLength = payload.size();
}

void Cerios::InternalComms::PacketImpl::serializeData(std::vector<std::uint8_t> &outputBuffer) {
    std::size_t currentLength = outputBuffer.size();
    outputBuffer.resize(outputBuffer.size() + sizeof(MessagePacketHeader));
    std::memcpy(this->packetHeader.get(), outputBuffer.data() + currentLength, sizeof(MessagePacketHeader));
    if (this->packetHeader->payloadLength > 0) {
        std::copy(this->payload->begin(), this->payload->end(), std::back_inserter(outputBuffer));
    }
}

Cerios::InternalComms::MessageID Cerios::InternalComms::PacketImpl::getMessageID() {
    return static_cast<MessageID>(this->packetHeader->id);
}

std::weak_ptr<std::vector<std::uint8_t>> Cerios::InternalComms::PacketImpl::getPayload() {
    return std::weak_ptr<std::vector<std::uint8_t>>(this->payload);
}

void Cerios::InternalComms::PacketImpl::setPacketNumber(std::uint8_t packetNumber) {
    this->packetHeader->packetNumber = packetNumber;
}

std::uint8_t Cerios::InternalComms::PacketImpl::getPacketNumber() {
    return this->packetHeader->packetNumber;
}