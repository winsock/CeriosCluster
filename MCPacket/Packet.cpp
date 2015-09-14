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
#include "LoginSuccessPacket.hpp"
#include "SetCompressionPacket.hpp"
#include "JoinGamePacket.hpp"
#include "KeepAlivePacket.hpp"
#include "SpawnPositionPacket.hpp"
#include "PlayerPositionAndLookPacket.hpp"
#include "PlayerAbilitiesPacket.hpp"

namespace {
    Cerios::Server::Packet::Registrar<Cerios::Server::HandshakePacket> handshake(Cerios::Server::ClientState::HANDSHAKE, 0x00);
    
    Cerios::Server::Packet::Registrar<Cerios::Server::ServerStatusPacket> status(Cerios::Server::ClientState::STATUS, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::PingPacket> ping(Cerios::Server::ClientState::STATUS, 0x01);
    
    Cerios::Server::Packet::Registrar<Cerios::Server::LoginStartPacket> loginStart(Cerios::Server::ClientState::LOGIN, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::EncryptionPacket> encryption(Cerios::Server::ClientState::LOGIN, 0x01);
    Cerios::Server::Packet::Registrar<Cerios::Server::LoginSuccessPacket> loginSuccess(Cerios::Server::ClientState::LOGIN, 0x02);
    Cerios::Server::Packet::Registrar<Cerios::Server::SetCompressionPacket> setCompressionLogin(Cerios::Server::ClientState::LOGIN, 0x03);
    
    Cerios::Server::Packet::Registrar<Cerios::Server::KeepAlivePacket> keepAlivePlay(Cerios::Server::ClientState::PLAY, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::JoinGamePacket> joinGame(Cerios::Server::ClientState::PLAY, 0x01);
    Cerios::Server::Packet::Registrar<Cerios::Server::SpawnPositionPacket> spawnPosition(Cerios::Server::ClientState::PLAY, 0x05);
    Cerios::Server::Packet::Registrar<Cerios::Server::PlayerPositionAndLookPacket> playerLookPos(Cerios::Server::ClientState::PLAY, 0x08);
    Cerios::Server::Packet::Registrar<Cerios::Server::PlayerAbilitiesPacket> playerAbilities(Cerios::Server::ClientState::PLAY, 0x39);
    Cerios::Server::Packet::Registrar<Cerios::Server::SetCompressionPacket> setCompressionPlay(Cerios::Server::ClientState::PLAY, 0x46);
}

Cerios::Server::Packet::packet_registry &Cerios::Server::Packet::registry() {
    static Cerios::Server::Packet::packet_registry impl;
    return impl;
}

void Cerios::Server::Packet::compressIfLargerThan(std::size_t lengthBytes) {
    if (this->rawPayload.size() > lengthBytes) {
        this->compressPacket();
    }
}

void Cerios::Server::Packet::compressPacket() {
    if (this->compressed) {
        return; // We already compressed this packet
    }
    
    Cerios::Server::Packet::compressData(this->rawPayload);
    this->compressed = true;
}

void Cerios::Server::Packet::compressData(std::vector<std::uint8_t> &data) {
    std::size_t inflatedSize = data.size();
    std::vector<std::uint8_t> compressedBuffer(inflatedSize);
    z_stream zStream;
    
    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;
    zStream.opaque = Z_NULL;
    
    if (deflateInit(&zStream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        return;
    }
    
    std::int32_t returnCode;
    zStream.avail_in = static_cast<std::uint32_t>(data.size());
    zStream.next_in = data.data();
    
    std::size_t offset = 0;
    std::size_t compressedSize = 0;
    zStream.avail_out = 0;
    
    do {
        if (zStream.avail_in > 0 && zStream.avail_out <= 0) {
            // We have more data and we ran out of buffer!
            zStream.avail_out = static_cast<std::uint32_t>(inflatedSize);
            zStream.next_out = compressedBuffer.data() + offset;
        }
        
        std::size_t originalOutputSize = zStream.avail_out;
        
        returnCode = deflate(&zStream, Z_FINISH);
        
        compressedSize += originalOutputSize - zStream.avail_out;
        
        if (zStream.avail_in > 0 && zStream.avail_out <=0) {
            // We have more data and we ran out of buffer!
            // Update the offset in preperation for expanding the vector
            offset += inflatedSize;
        }
    } while (returnCode != Z_STREAM_ERROR && zStream.avail_in > 0 && returnCode != Z_STREAM_END);
    
    // resize down to the actual used size
    compressedBuffer.resize(compressedSize);
    deflateEnd(&zStream);
    // Write inflated size
    Cerios::Server::Packet::writeVarIntToFront(compressedBuffer, static_cast<std::int32_t>(inflatedSize));
    // Set the data buffer the compressed data + length
    data = compressedBuffer;
}

void Cerios::Server::Packet::framePacket() {
    Cerios::Server::Packet::writeBufferLengthToFront(this->rawPayload);
}

Cerios::Server::Packet::packet_data_store &Cerios::Server::Packet::getData() {
    return this->rawPayload;
}

void Cerios::Server::Packet::resetBuffer() {
    this->compressed = false;
    this->rawPayload.clear();
}

void Cerios::Server::Packet::writeVarIntToBuffer(std::uint32_t input) {
    while ((input & -128) != 0) {
        this->rawPayload.push_back(static_cast<std::uint8_t>((input & 127) | 128));
        input >>= 7;
    }
    this->rawPayload.push_back(static_cast<std::uint8_t>(input));
}

void Cerios::Server::Packet::writeVarLongToBuffer(std::uint64_t input) {
    while ((input & -128L) != 0L) {
        this->rawPayload.push_back(static_cast<std::uint8_t>((input & 127) | 128));
        input >>= 7;
    }
    this->rawPayload.push_back(static_cast<std::uint8_t>(input));
}

std::size_t getVarIntSize(std::int64_t input) {
    for (std::size_t bytes = 1; bytes < 5; ++bytes) {
        if ((input & -1 << bytes * 7) == 0) {
            return bytes;
        }
    }
    return 5;
}

void Cerios::Server::Packet::serializeToBuffer(Cerios::Server::Side sideSending, std::vector<std::uint8_t> &buffer) {
    this->serializePacket(sideSending);
    std::copy(this->rawPayload.begin(), this->rawPayload.end(), std::back_inserter(buffer));
}

const void Cerios::Server::Packet::writeBufferLengthToFront(std::vector<std::uint8_t> &buffer) {
    if (getVarIntSize(buffer.size()) > Cerios::Server::Packet::MAX_LENGTH_BYTES) {
        throw std::runtime_error("Packet size too large to prepend! Packet size: " + std::to_string(getVarIntSize(buffer.size())));
    }
    Cerios::Server::Packet::writeVarIntToFront(buffer, static_cast<std::int32_t>(buffer.size()));
}

const void Cerios::Server::Packet::writeVarIntToFront(std::vector<std::uint8_t> &buffer, std::int32_t input) {
    std::vector<std::uint8_t> bytes;
    while ((input & -128) != 0) {
        bytes.push_back(static_cast<std::uint8_t>((input & 127) | 128));
        input >>= 7;
    }
    bytes.push_back(static_cast<std::uint8_t>(input & 127));
    buffer.insert(buffer.begin(), bytes.begin(), bytes.end());
}

const std::size_t Cerios::Server::Packet::readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::uint8_t> &buffer, bool consume) {
    (*intOut) = 0;
    int numByte = 0;
    int8_t byte;
    
    if (buffer.size() <= 0) {
        return 0;
    }
    
    do {
        if (numByte >= buffer.size()) {
            return 0;
        }
        
        byte = buffer[numByte];
        (*intOut) |= (byte & 127) << numByte++ * 7;
        
        if (numByte > 5) {
            throw std::runtime_error("VarInt too big");
        }
    } while ((byte & 128) == 128);
    if (consume) {
        buffer.erase(buffer.begin(), buffer.begin() + numByte);
    }
    
    return numByte;
}

Cerios::Server::Packet::Packet(std::size_t length, std::vector<std::uint8_t> &buffer, Cerios::Server::ClientState state, bool consumeData) {
    std::size_t idSize = Cerios::Server::Packet::readVarIntFromBuffer(&this->packetId, buffer);
    std::copy(buffer.begin() + idSize, buffer.begin() + length, std::back_inserter(this->rawPayload));
    if (consumeData) {
        // Length includes the packet id.
        buffer.erase(buffer.begin(), buffer.begin() + length);
    }
}