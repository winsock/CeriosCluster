//
//  ChatMessagePacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/16/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "ChatMessagePacket.hpp"

Cerios::Server::ChatMessagePacket::ChatMessagePacket(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    std::int32_t jsonChatDataLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&jsonChatDataLength, this->rawPayload, true);
    if (this->rawPayload.size() >= jsonChatDataLength) {
        this->jsonChatData = std::string(this->rawPayload.begin(), this->rawPayload.begin() + jsonChatDataLength);
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + jsonChatDataLength);
    }
    if (side == Cerios::Server::Side::CLIENT_BOUND) {
        this->readPODFromBuffer<ChatType>(this->chatPosition);
    }
    this->resetBuffer();
}

Cerios::Server::ChatMessagePacket::ChatMessagePacket(Cerios::Server::Side side) : Packet(side == Side::SERVER_BOUND ? 0x01 : 0x02) { }

void Cerios::Server::ChatMessagePacket::serializePacket(Cerios::Server::Side sideFrom) {
    Packet::serializePacket(sideFrom);
    this->writeVarIntToBuffer(static_cast<std::int32_t>(this->jsonChatData.size()));
    std::copy(this->jsonChatData.data(), this->jsonChatData.data() + this->jsonChatData.size(), std::back_inserter(this->rawPayload));
    if (sideFrom == Cerios::Server::Side::CLIENT_BOUND) {
        this->writePODToBuffer(this->chatPosition);
    }
}