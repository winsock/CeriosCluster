//
//  JoinGamePacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/2/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "JoinGamePacket.hpp"
#include "AbstractClient.hpp"

Cerios::Server::JoinGamePacket::JoinGamePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    if (this->rawPayload.size() >= sizeof(this->playerEntityId)) {
        std::memcpy(&this->playerEntityId, this->rawPayload.data(), sizeof(this->playerEntityId));
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sizeof(this->playerEntityId));
    }
    if (this->rawPayload.size() >= sizeof(this->gamemode)) {
        std::memcpy(&this->gamemode, this->rawPayload.data(), sizeof(this->gamemode));
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sizeof(this->gamemode));
    }
    if (this->rawPayload.size() >= sizeof(this->dimensionId)) {
        std::memcpy(&this->dimensionId, this->rawPayload.data(), sizeof(this->dimensionId));
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sizeof(this->dimensionId));
    }
    if (this->rawPayload.size() >= sizeof(this->difficulty)) {
        std::memcpy(&this->difficulty, this->rawPayload.data(), sizeof(this->difficulty));
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sizeof(this->difficulty));
    }
    if (this->rawPayload.size() >= sizeof(this->maxPlayersOnPlayerList)) {
        std::memcpy(&this->maxPlayersOnPlayerList, this->rawPayload.data(), sizeof(this->maxPlayersOnPlayerList));
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sizeof(this->maxPlayersOnPlayerList));
    }
    std::int32_t levelTypeStringLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&levelTypeStringLength, &this->rawPayload, true);
    if (this->rawPayload.size() >= levelTypeStringLength) {
        this->levelType = std::string(this->rawPayload.begin(), this->rawPayload.begin() + levelTypeStringLength);
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + levelTypeStringLength);
    }
    if (this->rawPayload.size() >= sizeof(this->reducedDebugInfo)) {
        std::memcpy(&this->reducedDebugInfo, this->rawPayload.data(), sizeof(this->reducedDebugInfo));
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sizeof(this->reducedDebugInfo));
    }
    this->rawPayload.clear();
}

Cerios::Server::JoinGamePacket::JoinGamePacket() : Packet(0x01), playerEntityId(0), gamemode(0), dimensionId(0), difficulty(0), maxPlayersOnPlayerList(255), levelType("default"), reducedDebugInfo(false) {
}

void Cerios::Server::JoinGamePacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    Cerios::Server::Packet::write32bitInt(this->playerEntityId);
    Cerios::Server::Packet::writeByte(this->gamemode);
    Cerios::Server::Packet::writeByte(this->dimensionId);
    Cerios::Server::Packet::writeByte(this->difficulty);
    Cerios::Server::Packet::writeByte(this->maxPlayersOnPlayerList);
    Cerios::Server::Packet::writeVarIntToBuffer(static_cast<std::int32_t>(this->levelType.size()));
    std::copy(this->levelType.data(), this->levelType.data() + this->levelType.size(), std::back_inserter(this->rawPayload));
}