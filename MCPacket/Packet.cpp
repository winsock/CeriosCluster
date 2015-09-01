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

#include "HandshakePacket.hpp"
#include "ServerStatusPacket.cpp"
#include "PingPacket.hpp"
#include "LoginStartPacket.hpp"
#include "EncryptionPacket.hpp"

namespace {
    Cerios::Server::Packet::Registrar<Cerios::Server::HandshakePacket> handshake(Cerios::Server::ClientState::HANDSHAKE, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::ServerStatusPacket> status(Cerios::Server::ClientState::STATUS, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::PingPacket> ping(Cerios::Server::ClientState::STATUS, 0x01);
    Cerios::Server::Packet::Registrar<Cerios::Server::LoginStartPacket> loginStart(Cerios::Server::ClientState::LOGIN, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::EncryptionPacket> encryption(Cerios::Server::ClientState::LOGIN, 0x01);
}

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

void Cerios::Server::Packet::writeVarIntToBuffer(std::uint32_t input) {
    while ((input & -128) != 0) {
        this->rawPayload.push_back(static_cast<std::int8_t>((input & 127) | 128));
        input >>= 7;
    }
    this->rawPayload.push_back(static_cast<std::int8_t>(input));
}

void Cerios::Server::Packet::writeVarLongToBuffer(std::uint64_t input) {
    while ((input & -128L) != 0L) {
        this->rawPayload.push_back(static_cast<std::int8_t>((input & 127) | 128));
        input >>= 7;
    }
    this->rawPayload.push_back(static_cast<std::int8_t>(input));
}

std::size_t getVarIntSize(std::int64_t input) {
    for (std::size_t bytes = 1; bytes < 5; ++bytes) {
        if ((input & -1 << bytes * 7) == 0) {
            return bytes;
        }
    }
    return 5;
}

void Cerios::Server::Packet::writeBufferLengthToFront() {
    if (getVarIntSize(this->rawPayload.size()) > Cerios::Server::Packet::MAX_LENGTH_BYTES) {
        throw std::runtime_error("Packet size too large to prepend! Packet size: " + std::to_string(getVarIntSize(this->rawPayload.size())));
    }
    
    std::vector<std::int8_t> lengthBytes;
    std::uint64_t input(this->rawPayload.size());
    while ((input & -128) != 0) {
        lengthBytes.push_back(static_cast<std::int8_t>((input & 127) | 128));
        input >>= 7;
    }
    lengthBytes.push_back(static_cast<std::int8_t>(input & 127));
    this->rawPayload.insert(this->rawPayload.begin(), lengthBytes.begin(), lengthBytes.end());
}

void Cerios::Server::Packet::write64bitInt(std::int64_t input) {
    std::copy(&input, &input + sizeof(input), std::back_inserter(this->rawPayload));
}

const std::size_t Cerios::Server::Packet::readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::int8_t> *buffer, bool consume) {
    return Cerios::Server::Packet::readVarIntFromBuffer(intOut, buffer, 0, consume);
}

Cerios::Server::Packet::Packet(std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer,Cerios::Server::ClientState state, bool consumeData) {
    std::size_t idSize = Cerios::Server::Packet::readVarIntFromBuffer(&this->packetId, buffer.get());
    std::copy(buffer->begin() + idSize, buffer->begin() + length, std::back_inserter(this->rawPayload));
    if (consumeData) {
        // Length includes the packet id.
        buffer->erase(buffer->begin(), buffer->begin() + length);
    }
}

std::shared_ptr<Cerios::Server::Packet> Cerios::Server::Packet::newPacket(Cerios::Server::Side side, Cerios::Server::ClientState state, std::int32_t packetId) {
    return Packet::instantiateNew(side, state, packetId);
}

std::shared_ptr<Cerios::Server::Packet> Cerios::Server::Packet::parsePacket(Cerios::Server::Side side, std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer, ClientState state, bool consumeData) {
    return Packet::instantiateFromData(side, state, std::shared_ptr<Cerios::Server::Packet>(new Cerios::Server::Packet(length, buffer, state, consumeData)));
}