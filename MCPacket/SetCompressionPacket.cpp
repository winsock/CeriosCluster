//
//  SetCompressionPacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/2/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "SetCompressionPacket.hpp"

Cerios::Server::SetCompressionPacket::SetCompressionPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    Cerios::Server::Packet::readVarIntFromBuffer(&this->compressionThreshold, this->rawPayload, true);
    this->resetBuffer();
}

Cerios::Server::SetCompressionPacket::SetCompressionPacket() : Packet(0x01), compressionThreshold(-1) {
}

void Cerios::Server::SetCompressionPacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    Cerios::Server::Packet::writeVarIntToBuffer(this->compressionThreshold);
}