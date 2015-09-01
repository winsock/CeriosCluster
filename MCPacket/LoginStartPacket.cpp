//
//  LoginStartPacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 8/31/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "LoginStartPacket.hpp"
#include "AbstractClient.hpp"

Cerios::Server::LoginStartPacket::LoginStartPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    std::int32_t stringLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&stringLength, &this->rawPayload, true);
    if (this->rawPayload.size() >= stringLength) {
        this->playerName = std::string(this->rawPayload.begin(), this->rawPayload.begin() + stringLength);
    }
    this->rawPayload.clear();
}

void Cerios::Server::LoginStartPacket::sendTo(Cerios::Server::AbstractClient *client) {
    this->serializePacket(client->getSide());
    client->sendData(this->rawPayload);
}

void Cerios::Server::LoginStartPacket::serializePacket(Cerios::Server::Side sideFrom) {
    Packet::serializePacket(sideFrom);
    this->writeVarIntToBuffer(static_cast<std::int32_t>(this->playerName.size()));
    std::copy(this->playerName.begin(), this->playerName.end(), std::back_inserter(this->rawPayload));
    this->writeBufferLengthToFront();
}