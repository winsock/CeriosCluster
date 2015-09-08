//
//  LoginSuccessPacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/2/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "LoginSuccessPacket.hpp"
#include "AbstractClient.hpp"

Cerios::Server::LoginSuccessPacket::LoginSuccessPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    std::int32_t uuidLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&uuidLength, &this->rawPayload, true);
    if (this->rawPayload.size() >= uuidLength) {
        this->uuid = std::string(this->rawPayload.begin(), this->rawPayload.begin() + uuidLength);
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + uuidLength);
    }
    std::int32_t usernameLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&usernameLength, &this->rawPayload, true);
    if (this->rawPayload.size() - uuidLength >= usernameLength) {
        this->username = std::string(this->rawPayload.begin(), this->rawPayload.begin() + usernameLength);
    }
    this->rawPayload.clear();
}

Cerios::Server::LoginSuccessPacket::LoginSuccessPacket() : Packet(0x02) {
}

void Cerios::Server::LoginSuccessPacket::serializePacket(Cerios::Server::Side sideFrom) {
    Packet::serializePacket(sideFrom);
    this->writeVarIntToBuffer(static_cast<std::int32_t>(this->uuid.size()));
    std::copy(this->uuid.data(), this->uuid.data() + this->uuid.size(), std::back_inserter(this->rawPayload));
    this->writeVarIntToBuffer(static_cast<std::int32_t>(this->username.size()));
    std::copy(this->username.data(), this->username.data() + this->username.size(), std::back_inserter(this->rawPayload));
}