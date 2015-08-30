//
//  ServerStatusRequestPacket.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/29/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "ServerStatusPacket.hpp"
#include "AbstractClient.hpp"

#include <iostream>

Cerios::Server::ServerStatusPacket::ServerStatusPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    this->rawPayload.clear();
}

Cerios::Server::ServerStatusPacket::ServerStatusPacket() : Packet(0x00) {
    
}

void Cerios::Server::ServerStatusPacket::onReceivedBy(Cerios::Server::AbstractClient *client) {
    std::shared_ptr<Cerios::Server::Packet> response = Packet::instantiateNew(client->getState(), 0x00);
    Packet::onReceivedBy(client);
    response->sendTo(client);
}

void Cerios::Server::ServerStatusPacket::sendTo(Cerios::Server::AbstractClient *client) {
    this->jsonEncodedServerStatus = "{"
    "\"version\": {"
    "    \"name\": \"1.8.8\","
    "    \"protocol\": 47"
    "},"
    "\"players\": {"
    "    \"max\": 10000,"
    "    \"online\": 0"
    "},"
    "\"description\": {"
    "    \"text\": \"Hello World!\""
    "}"
    "}";
    this->serializePacket();
    client->sendData(this->rawPayload);
}

void Cerios::Server::ServerStatusPacket::serializePacket() {
    this->rawPayload.clear();
    this->writeVarIntToBuffer(this->packetId);
    this->writeVarIntToBuffer(static_cast<std::int32_t>(this->jsonEncodedServerStatus.size()));
    std::copy(this->jsonEncodedServerStatus.begin(), this->jsonEncodedServerStatus.end(), std::back_inserter(this->rawPayload));
    this->writeBufferLengthToFront();
}