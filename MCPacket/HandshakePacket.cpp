//
//  ServerHandshakePacket.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/29/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "HandshakePacket.hpp"

#include <iostream>

Cerios::Server::HandshakePacket::HandshakePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    Cerios::Server::Packet::readVarIntFromBuffer(&this->protocolVersion, this->rawPayload, true);
    std::int32_t stringLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&stringLength, this->rawPayload, true);
    this->serverAddress = std::string(this->rawPayload.begin(), this->rawPayload.begin() + stringLength);
    this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + stringLength);
    this->serverPort = this->readPODFromBuffer<std::uint16_t>(25565);
    Cerios::Server::Packet::readVarIntFromBuffer(reinterpret_cast<std::int32_t *>(&this->requestedNextState), this->rawPayload, true);
    this->resetBuffer();
}

Cerios::Server::HandshakePacket::HandshakePacket() : Packet(0x00) { }