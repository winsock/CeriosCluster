//
//  Packet.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/29/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "Packet.hpp"
#include <algorithm>
#include <iterator>

Cerios::Server::Packet::packet_registry &Cerios::Server::Packet::registry() {
    static Cerios::Server::Packet::packet_registry impl;
    return impl;
}

const std::size_t Cerios::Server::Packet::readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::int8_t> *buffer, std::size_t offset, bool consume) {
    (*intOut) = 0;
    int numByte = 0;
    int8_t byte;
    
    do {
        if (offset + numByte >= buffer->size()) {
            return 0;
        }
        
        byte = (*buffer)[offset + numByte];
        (*intOut) |= (byte & 127) << numByte++ * 7;
        
        if (numByte > 5) {
            throw std::runtime_error("VarInt too big");
        }
    } while ((byte & 128) == 128);
    if (consume) {
        buffer->erase(buffer->begin(), buffer->begin() + numByte);
    }
    
    return numByte;
}

void Cerios::Server::Packet::writeVarIntToBuffer(std::int32_t input) {
    while ((input & -128) != 0) {
        this->rawPayload.push_back((input & 127) | 128);
        input = static_cast<std::uint32_t>(input) >> 7;
    }
    this->rawPayload.push_back(static_cast<std::int8_t>(input));
}

void Cerios::Server::Packet::writeVarLongToBuffer(std::int64_t input) {
    while ((input & -128L) != 0L) {
        this->rawPayload.push_back((input & 127) | 128);
        input = static_cast<std::uint64_t>(input) >> 7;
    }
    this->rawPayload.push_back(static_cast<std::int8_t>(input));
}

void Cerios::Server::Packet::writeBufferLengthToFront() {
    packet_data_store::iterator position = this->rawPayload.begin();
    std::int32_t input(this->packetId);
    while ((input & -128) != 0) {
        position = this->rawPayload.insert(position, (input & 127) | 128);
        input = static_cast<std::uint32_t>(input) >> 7;
    }
    this->rawPayload.insert(position, (input & 127) | 128);
}

const std::size_t Cerios::Server::Packet::readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::int8_t> *buffer, bool consume) {
    return Cerios::Server::Packet::readVarIntFromBuffer(intOut, buffer, 0, consume);
}

Cerios::Server::Packet::Packet(std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer,Cerios::Server::ClientState state, bool consumeData) {
    std::size_t idSize = Cerios::Server::Packet::readVarIntFromBuffer(&this->packetId, buffer.get());
    if (length > 0) {
        std::copy(buffer->begin() + idSize, buffer->begin() + length, std::back_inserter(this->rawPayload));
        if (consumeData) {
            // Length includes the packet id.
            buffer->erase(buffer->begin(), buffer->begin() + length);
        }
    }
}

std::shared_ptr<Cerios::Server::Packet> Cerios::Server::Packet::newPacket(Cerios::Server::ClientState state, std::int32_t packetId) {
    return Packet::instantiateNew(state, packetId);
}

std::shared_ptr<Cerios::Server::Packet> Cerios::Server::Packet::parsePacket(std::size_t length, std::shared_ptr<std::vector<std::int8_t> > buffer, ClientState state, bool consumeData) {
    std::shared_ptr<Cerios::Server::Packet> tempPacket(new Cerios::Server::Packet(length, buffer, state, consumeData));
    return Packet::instantiateFromData(state, tempPacket);
}