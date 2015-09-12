//
//  PingPacket.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/30/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "PingPacket.hpp"

#include <chrono>

Cerios::Server::PingPacket::PingPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    this->unixEpoch = this->readPODFromBuffer<std::int64_t>(std::chrono::milliseconds(std::time(NULL)).count());
    this->resetBuffer();
}

Cerios::Server::PingPacket::PingPacket() : Packet(0x01) {
    this->unixEpoch = std::chrono::milliseconds(std::time(NULL)).count();
}

void Cerios::Server::PingPacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    this->writePODToBuffer(this->unixEpoch);
}