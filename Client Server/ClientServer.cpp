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
#include <vector>

#include "GameState.hpp"

Cerios::Server::ClientServer::ClientServer(std::uint16_t receivePort, bool ipv6) : service(new asio::io_service()),
sendSocket(*service),
receiveSocket(*service){
    sendSocket.open(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4());
    sendSocket.set_option(asio::socket_base::reuse_address(true));
    sendSocket.set_option(asio::socket_base::broadcast(true));
    sendSocket.bind(asio::ip::udp::endpoint(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4(), 0));
    
    receiveSocket.open(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4());
    receiveSocket.set_option(asio::socket_base::reuse_address(true));
    receiveSocket.bind(asio::ip::udp::endpoint(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4(), receivePort));
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
    this->receiveSocket.async_wait(asio::socket_base::wait_read, std::bind(&Cerios::Server::ClientServer::onDatagramMessageReceived, this, std::placeholders::_1));
}

void Cerios::Server::ClientServer::onDatagramMessageReceived(const asio::error_code &error) {
    if (error) {
        std::cerr<<error.message()<<std::endl;
        return;
    }
    
    std::size_t bytesAvailable = this->receiveSocket.available();
    std::vector<std::uint8_t> data(bytesAvailable);
    
    asio::ip::udp::endpoint senderEndpoint;
    asio::error_code receiveError;
    std::size_t packetSize = this->receiveSocket.receive_from(asio::buffer(data, bytesAvailable), senderEndpoint, 0, receiveError);
    if (receiveError) {
        std::cerr<<receiveError.message()<<std::endl;
        return;
    }
    
    data.resize(packetSize);
    this->handleMessage(senderEndpoint, Cerios::InternalComms::Packet::fromData(data));
    this->startReceive();
}

void Cerios::Server::ClientServer::handleMessage(asio::ip::udp::endpoint &endpointFrom, std::shared_ptr<Cerios::InternalComms::Packet> packet) {
    std::cout<<static_cast<std::uint8_t>(packet->getMessageID())<<std::endl;
    switch (packet->getMessageID()) {
        case Cerios::InternalComms::MessageID::ACCEPT_CLIENT:
            std::cout<<"Accept Client"<<std::endl;
            break;
        default:
            break;
    }
}

Cerios::Server::ClientServer::~ClientServer() {
    
}