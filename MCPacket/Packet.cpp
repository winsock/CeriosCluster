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

namespace {
    Cerios::Server::Packet::Registrar<Cerios::Server::HandshakePacket> handshake(Cerios::Server::ClientState::HANDSHAKE, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::ServerStatusPacket> status(Cerios::Server::ClientState::STATUS, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::PingPacket> ping(Cerios::Server::ClientState::STATUS, 0x01);
    Cerios::Server::Packet::Registrar<Cerios::Server::LoginStartPacket> loginStart(Cerios::Server::ClientState::LOGIN, 0x00);
    Cerios::Server::Packet::Registrar<Cerios::Server::EncryptionPacket> encryption(Cerios::Server::ClientState::LOGIN, 0x01);
    Cerios::Server::Packet::Registrar<Cerios::Server::LoginSuccessPacket> loginSuccess(Cerios::Server::ClientState::LOGIN, 0x02);
    Cerios::Server::Packet::Registrar<Cerios::Server::SetCompressionPacket> setCompressionLogin(Cerios::Server::ClientState::LOGIN, 0x03);
    Cerios::Server::Packet::Registrar<Cerios::Server::SetCompressionPacket> setCompressionPlay(Cerios::Server::ClientState::PLAY, 0x46);
    Cerios::Server::Packet::Registrar<Cerios::Server::JoinGamePacket> joinGame(Cerios::Server::ClientState::PLAY, 0x01);
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

void Cerios::Server::Packet::sendTo(Cerios::Server::AbstractClient *client, std::int32_t compressionThreshold) {
    this->serializePacket(client->getSide());
    if (compressionThreshold >= 0 && this->rawPayload.size() > compressionThreshold) {
        std::size_t inflatedSize = this->rawPayload.size();
        std::vector<std::int8_t> compressedBuffer(inflatedSize);
        z_stream zStream;
        
        zStream.zalloc = Z_NULL;
        zStream.zfree = Z_NULL;
        zStream.opaque = Z_NULL;
        
        if (deflateInit(&zStream, Z_DEFAULT_COMPRESSION) != Z_OK) {
            return;
        }
        
        std::int32_t returnCode;
        zStream.avail_in = static_cast<std::uint32_t>(this->rawPayload.size());
        zStream.next_in = reinterpret_cast<std::uint8_t *>(this->rawPayload.data());
        
        std::size_t offset = 0;
        std::size_t compressedSize = 0;
        zStream.avail_out = 0;
        
        do {
            if (zStream.avail_in > 0 && zStream.avail_out <= 0) {
                // We have more data and we ran out of buffer!
                zStream.avail_out = static_cast<std::uint32_t>(inflatedSize);
                zStream.next_out = reinterpret_cast<std::uint8_t *>(compressedBuffer.data() + offset);
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
        this->writeVarIntToFront(&compressedBuffer, static_cast<std::int32_t>(inflatedSize));
        // Make the payload the compressed data + length
        this->rawPayload = compressedBuffer;
        // Fall through for writing the full packet length and sending
    }
    this->writeBufferLengthToFront();
    client->sendData(this->rawPayload);
}

void Cerios::Server::Packet::writeBufferLengthToFront() {
    this->writeBufferLengthToFront(&this->rawPayload);
}

void Cerios::Server::Packet::writeBufferLengthToFront(std::vector<std::int8_t> *buffer) {
    if (getVarIntSize(buffer->size()) > Cerios::Server::Packet::MAX_LENGTH_BYTES) {
        throw std::runtime_error("Packet size too large to prepend! Packet size: " + std::to_string(getVarIntSize(this->rawPayload.size())));
    }
    this->writeVarIntToFront(buffer, static_cast<std::int32_t>(buffer->size()));
}

void Cerios::Server::Packet::writeVarIntToFront(std::vector<std::int8_t> *buffer, std::int32_t input) {
    std::vector<std::int8_t> bytes;
    while ((input & -128) != 0) {
        bytes.push_back(static_cast<std::int8_t>((input & 127) | 128));
        input >>= 7;
    }
    bytes.push_back(static_cast<std::int8_t>(input & 127));
    buffer->insert(buffer->begin(), bytes.begin(), bytes.end());
}

void Cerios::Server::Packet::write64bitInt(std::int64_t input) {
    std::copy(&input, &input + sizeof(input), std::back_inserter(this->rawPayload));
}

void Cerios::Server::Packet::write32bitInt(std::int32_t input) {
    std::copy(&input, &input + sizeof(input), std::back_inserter(this->rawPayload));
}

void Cerios::Server::Packet::writeByte(std::int8_t input) {
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