//
//  KeepAlivePacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/13/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "KeepAlivePacket.hpp"

#include <random>

Cerios::Server::KeepAlivePacket::KeepAlivePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    this->readVarIntFromBuffer(&this->keepAliveId, this->rawPayload);
    this->resetBuffer();
}

Cerios::Server::KeepAlivePacket::KeepAlivePacket() : Packet(0x00) {
    static std::function<std::int32_t(void)> randomEngine = std::bind(std::uniform_int_distribution<>(0, INT32_MAX), std::mt19937(std::random_device()()));
    this->keepAliveId = randomEngine();
}

void Cerios::Server::KeepAlivePacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    this->writeVarIntToBuffer(this->keepAliveId);
}