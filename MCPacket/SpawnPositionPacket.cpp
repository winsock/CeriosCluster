//
//  SpawnPositionPacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/14/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "SpawnPositionPacket.hpp"

Cerios::Server::SpawnPositionPacket::SpawnPositionPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    this->readPODFromBuffer<std::uint64_t>(this->spawnPosition.rawPosition);
    this->resetBuffer();
}

Cerios::Server::SpawnPositionPacket::SpawnPositionPacket() : Packet(0x05) {
}

void Cerios::Server::SpawnPositionPacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    this->writePODToBuffer(this->spawnPosition.rawPosition);
}