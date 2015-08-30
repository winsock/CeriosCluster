//
//  ServerHandshakePacket.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/29/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "HandshakePacket.hpp"
#include "AbstractClient.hpp"

#include <iostream>

Cerios::Server::HandshakePacket::HandshakePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    Cerios::Server::Packet::readVarIntFromBuffer(&this->protocolVersion, &this->rawPayload, true);
    std::int32_t stringLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&stringLength, &this->rawPayload, true);
    this->serverAddress = std::string(this->rawPayload.begin(), this->rawPayload.begin() + stringLength);
    this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + stringLength);
    std::memcpy(&this->serverPort, this->rawPayload.data(), sizeof(std::uint16_t));
    this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sizeof(std::uint16_t));
    Cerios::Server::Packet::readVarIntFromBuffer(reinterpret_cast<std::int32_t *>(&this->requestedNextState), &this->rawPayload, true);
    this->rawPayload.clear();
}

Cerios::Server::HandshakePacket::HandshakePacket() : Packet(0x00) { }