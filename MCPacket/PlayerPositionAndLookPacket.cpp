//
//  PlayerPositionAndLookPacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/14/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "PlayerPositionAndLookPacket.hpp"

Cerios::Server::PlayerPositionAndLookPacket::PlayerPositionAndLookPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    this->readPODFromBuffer<std::double_t>(this->x);
    this->readPODFromBuffer<std::double_t>(this->y);
    this->readPODFromBuffer<std::double_t>(this->z);
    
    this->readPODFromBuffer<std::float_t>(this->yaw);
    this->readPODFromBuffer<std::float_t>(this->pitch);

    this->readPODFromBuffer<std::uint8_t>(this->flags.rawFlags);
    this->resetBuffer();
}

Cerios::Server::PlayerPositionAndLookPacket::PlayerPositionAndLookPacket() : Packet(0x08), x(0.0), y(0.0), z(0.0), yaw(270.0), pitch(0.0) {
    flags.x = true;
    flags.y = true;
    flags.z = true;
}

void Cerios::Server::PlayerPositionAndLookPacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    
    this->writePODToBuffer(this->x);
    this->writePODToBuffer(this->y);
    this->writePODToBuffer(this->z);

    this->writePODToBuffer(this->yaw);
    this->writePODToBuffer(this->pitch);

    this->writePODToBuffer(this->flags.rawFlags);
}