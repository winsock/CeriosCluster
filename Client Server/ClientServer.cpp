//
//  ClientServer.cpp
//  Client Server
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "ClientServer.hpp"

#include <cstdlib>
#include <iostream>

#include "GameState.hpp"

Cerios::Server::ClientServer::ClientServer(unsigned short nodeCommsPort, bool ipv6) : service(new asio::io_service()), socket(*service, asio::ip::udp::endpoint(asio::ip::address_v4::any(), nodeCommsPort)), messageBuffer(65536)  {
    
}

void Cerios::Server::ClientServer::init() {
    this->startReceive();
}

void Cerios::Server::ClientServer::listen() {
    try {
        this->service->run();
    } catch (std::exception e) {
        std::cerr<<"Caught error in main server event loop: "<<e.what()<<std::endl;
    }
}

void Cerios::Server::ClientServer::startReceive() {
    std::shared_ptr<asio::ip::udp::endpoint> messageEndpoint;
    this->socket.async_receive_from(asio::buffer(this->messageBuffer), *messageEndpoint, std::bind(&Cerios::Server::ClientServer::onDatagramMessageReceived, this, messageEndpoint, std::placeholders::_1, std::placeholders::_2));
}

void Cerios::Server::ClientServer::onDatagramMessageReceived(std::shared_ptr<asio::ip::udp::endpoint> messageEndpoint, const asio::error_code &error, std::size_t bytes_transferred) {
    MessagePacketHeader header;
    std::copy(this->messageBuffer.begin(), this->messageBuffer.begin() + sizeof(MessagePacketHeader), reinterpret_cast<std::uint8_t *>(&header));
    
    std::vector<std::uint8_t> packetData(header.payloadLength);
    std::copy_n(this->messageBuffer.begin() + sizeof(MessagePacketHeader), header.payloadLength, std::back_inserter(packetData));
    this->messageBuffer.clear();
    // Packet
    
    this->startReceive();
}

Cerios::Server::ClientServer::~ClientServer() {
    
}