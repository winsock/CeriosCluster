//
//  PingPacket.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/30/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "PingPacket.hpp"
#include "AbstractClient.hpp"

#include <chrono>

Cerios::Server::PingPacket::PingPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    if (this->rawPayload.size() >= sizeof(this->unixEpoch)) {
        std::memcpy(&this->unixEpoch, this->rawPayload.data(), sizeof(this->unixEpoch));
    }
    this->rawPayload.clear();
}

Cerios::Server::PingPacket::PingPacket() : Packet(0x01) {
    this->unixEpoch = std::chrono::milliseconds(std::time(NULL)).count();
}

void Cerios::Server::PingPacket::onReceivedBy(Cerios::Server::AbstractClient *client) {
    std::shared_ptr<Cerios::Server::Packet> response = Packet::instantiateNew(client->getState(), 0x01);
    Packet::onReceivedBy(client);
    response->sendTo(client);
}

void Cerios::Server::PingPacket::writeComplete(Cerios::Server::AbstractClient *client) {
    client->setState(ClientState::HANDSHAKE);
    client->disconnect();
}

void Cerios::Server::PingPacket::sendTo(Cerios::Server::AbstractClient *client) {
    this->serializePacket();
    client->sendData(this->rawPayload);
}

void Cerios::Server::PingPacket::serializePacket() {
    this->rawPayload.clear();
    this->writeVarIntToBuffer(this->packetId);
    this->write64bitInt(this->unixEpoch);
    this->writeBufferLengthToFront();
}