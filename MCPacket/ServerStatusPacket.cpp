//
//  ServerStatusRequestPacket.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/29/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "ServerStatusPacket.hpp"

#include <iostream>

Cerios::Server::ServerStatusPacket::ServerStatusPacket(std::shared_ptr<Cerios::Server::Packet> packetInProgress) : Packet(packetInProgress) {
    this->resetBuffer();
}

Cerios::Server::ServerStatusPacket::ServerStatusPacket() : Packet(0x00) {
    
}

void Cerios::Server::ServerStatusPacket::serializePacket(Cerios::Server::Side sideFrom) {
    Packet::serializePacket(sideFrom);
    this->writeVarIntToBuffer(static_cast<std::int32_t>(this->jsonEncodedServerStatus.size()));
    std::copy(this->jsonEncodedServerStatus.data(), this->jsonEncodedServerStatus.data() + this->jsonEncodedServerStatus.size(), std::back_inserter(this->rawPayload));
}