//
//  JoinGamePacket.cpp
//  MCPacket
//
//  Created by Andrew Querol on 9/2/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "JoinGamePacket.hpp"

Cerios::Server::JoinGamePacket::JoinGamePacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    this->playerEntityId = this->readPODFromBuffer<std::int32_t>(0);
    this->gamemode = this->readPODFromBuffer<std::uint8_t>(0);
    this->dimensionId = this->readPODFromBuffer<std::int8_t>(0);
    this->difficulty = this->readPODFromBuffer<std::uint8_t>(0);
    this->maxPlayersOnPlayerList = this->readPODFromBuffer<std::uint8_t>(255);

    std::int32_t levelTypeStringLength;
    Cerios::Server::Packet::readVarIntFromBuffer(&levelTypeStringLength, this->rawPayload, true);
    if (this->rawPayload.size() >= levelTypeStringLength) {
        this->levelType = std::string(this->rawPayload.begin(), this->rawPayload.begin() + levelTypeStringLength);
        this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + levelTypeStringLength);
    }
    
    this->reducedDebugInfo = this->readPODFromBuffer<bool>(false);
    this->resetBuffer();
}

Cerios::Server::JoinGamePacket::JoinGamePacket() : Packet(0x01), playerEntityId(0), gamemode(0), dimensionId(0), difficulty(0), maxPlayersOnPlayerList(255), levelType("default"), reducedDebugInfo(false) {
}

void Cerios::Server::JoinGamePacket::serializePacket(Cerios::Server::Side sideSending) {
    Packet::serializePacket(sideSending);
    this->writePODToBuffer(this->playerEntityId);
    this->writePODToBuffer(this->gamemode);
    this->writePODToBuffer(this->dimensionId);
    this->writePODToBuffer(this->difficulty);
    this->writePODToBuffer(this->maxPlayersOnPlayerList);
    Cerios::Server::Packet::writeVarIntToBuffer(static_cast<std::int32_t>(this->levelType.size()));
    std::copy(this->levelType.data(), this->levelType.data() + this->levelType.size(), std::back_inserter(this->rawPayload));
    this->writePODToBuffer(this->reducedDebugInfo);
}