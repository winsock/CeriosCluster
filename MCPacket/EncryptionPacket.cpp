//
//  EncryptionPacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 8/31/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "EncryptionPacket.hpp"
#include "AbstractClient.hpp"

Cerios::Server::EncryptionPacket::EncryptionPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    std::int32_t stringLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&stringLength, &this->rawPayload, true);
    if (this->rawPayload.size() >= stringLength) {
        this->serverId = std::string(this->rawPayload.begin(), this->rawPayload.begin() + stringLength);
    }
    this->rawPayload.clear();
}

Cerios::Server::EncryptionPacket::EncryptionPacket() : Packet(0x01) {
}

void Cerios::Server::EncryptionPacket::sendTo(Cerios::Server::AbstractClient *client) {
    this->serializePacket(client->getSide());
    client->sendData(this->rawPayload);
}

void Cerios::Server::EncryptionPacket::serializePacket(Cerios::Server::Side sideSending) {
    this->rawPayload.clear();

}