//
//  PlayerAbilitiesPacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/14/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "PlayerAbilitiesPacket.hpp"

Cerios::Server::PlayerAbilitiesPacket::PlayerAbilitiesPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    this->readPODFromBuffer<std::uint8_t>(this->abilities.rawFlags);
    
    this->readPODFromBuffer<std::float_t>(this->flyingSpeed);
    this->readPODFromBuffer<std::float_t>(this->walkingSpeed);
    
    this->resetBuffer();
}

Cerios::Server::PlayerAbilitiesPacket::PlayerAbilitiesPacket() : Packet(0x39), flyingSpeed(2.0), walkingSpeed(1.0) {
    abilities.godMode = true;
    abilities.canFly = true;
    abilities.isFlying = true;
}

void Cerios::Server::PlayerAbilitiesPacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    this->writePODToBuffer(this->abilities.rawFlags);
    this->writePODToBuffer(this->flyingSpeed);
    this->writePODToBuffer(this->walkingSpeed);
}