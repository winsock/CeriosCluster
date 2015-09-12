//
//  EncryptionPacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 8/31/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "EncryptionPacket.hpp"
#include <openssl/ssl.h>

Cerios::Server::EncryptionPacket::EncryptionPacket(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress), serverId("") {
    if (side == Side::SERVER) { // If packet is from the server going to the client, not priority to finish client side receipt processing.
        // Not complete
        std::int32_t stringLength;
        Cerios::Server::Packet::readVarIntFromBuffer(&stringLength, this->rawPayload, true);
        if (this->rawPayload.size() >= stringLength) {
            this->serverId = std::string(this->rawPayload.begin(), this->rawPayload.begin() + stringLength);
        }
        this->resetBuffer();
    } else {
        // From the client received on the server
        std::int32_t sharedSecretLength;
        Cerios::Server::Packet::readVarIntFromBuffer(&sharedSecretLength, this->rawPayload, true);
        std::copy(this->rawPayload.data(), this->rawPayload.data() + sharedSecretLength, std::back_inserter(this->sealedSharedSecret));
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sharedSecretLength);
        
        std::int32_t verifyTokenLength;
        Cerios::Server::Packet::readVarIntFromBuffer(&verifyTokenLength, this->rawPayload, true);
        std::copy(this->rawPayload.data(), this->rawPayload.data() + verifyTokenLength, std::back_inserter(this->sealedVerifyToken));
    }
    this->rawPayload.clear();
}

Cerios::Server::EncryptionPacket::EncryptionPacket(Cerios::Server::Side side) : Packet(0x01), serverId("") {
}

void Cerios::Server::EncryptionPacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    if (sideSending == Cerios::Server::Side::SERVER) {
        this->writeVarIntToBuffer(static_cast<std::int32_t>(this->serverId.size()));
        if (this->serverId.size() > 0) {
            std::copy(this->serverId.data(), this->serverId.data() + this->serverId.size(), std::back_inserter(this->rawPayload));
        }

        this->writeVarIntToBuffer(static_cast<std::int32_t>(this->publickKey.length()));
        std::copy(this->publickKey.data(), this->publickKey.data() + this->publickKey.size(), std::back_inserter(this->rawPayload));

        this->writeVarIntToBuffer(static_cast<std::int32_t>(this->clearVerifyToken.size()));
        std::copy(this->clearVerifyToken.data(), this->clearVerifyToken.data() + this->clearVerifyToken.size(), std::back_inserter(this->rawPayload));
    }
}