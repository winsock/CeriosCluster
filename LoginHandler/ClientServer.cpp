//
//  ClientServer.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/30/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "ClientServer.hpp"

#include <iostream>

#include "Client.hpp"
#include "LoginServer.hpp"

Cerios::Server::ClientServer::ClientServer(std::uint16_t messageListen, bool ipv6, std::shared_ptr<Cerios::Server::Login> owner) :
socket(new asio::ip::udp::socket(*owner->getIOService().lock(), asio::ip::udp::endpoint(asio::ip::address_v4::any(), messageListen))), owner(owner), messageBuffer(65536) {
    // Enable broadcast
    asio::socket_base::broadcast option(true);
    socket->set_option(option);
    
    this->startAsyncReceive();
}

void Cerios::Server::ClientServer::startAsyncReceive() {
    std::shared_ptr<asio::ip::udp::endpoint> messageEndpoint;

    this->socket->async_receive_from(asio::buffer(this->messageBuffer), *messageEndpoint, std::bind(&Cerios::Server::ClientServer::onDataReceived, this, messageEndpoint, std::placeholders::_1, std::placeholders::_2));
}

void Cerios::Server::ClientServer::onDataReceived(std::shared_ptr<asio::ip::udp::endpoint> messageEndpoint, const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Internal read error from clientserver node: "<<error.message()<<std::endl;
        return;
    } else {
        MessagePacketHeader header;
        std::copy(this->messageBuffer.begin(), this->messageBuffer.begin() + sizeof(MessagePacketHeader), reinterpret_cast<std::uint8_t *>(&header));
        
        std::vector<std::uint8_t> packetData(header.payloadLength);
        std::copy_n(this->messageBuffer.begin() + sizeof(MessagePacketHeader), header.payloadLength, std::back_inserter(packetData));
        this->messageBuffer.clear();
        // Packet
        
        this->startAsyncReceive();
    }
}

void Cerios::Server::ClientServer::onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Internal write error from clientserver node: "<<error.message()<<std::endl;
    }
}

void Cerios::Server::ClientServer::onWriteCompleteCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<std::function<void(void)>> callback) {
    onWriteComplete(error, bytes_transferred);
    (*callback)();
}

void Cerios::Server::ClientServer::sendShutdownSignal() {
    for (auto clientMapPair : this->clients) {
        clientMapPair.second->disconnect();
    }
    if (this->socket->is_open()) {
        std::shared_ptr<std::function<void(void)>> functionLambada(new std::function<void(void)>([this]() -> void{
            if (this->socket->is_open()) {
                this->socket->shutdown(asio::ip::tcp::socket::shutdown_both);
                this->socket->close();
            }
        }));
//        asio::async_write(*this->socket, asio::buffer(std::string("bye")), std::bind(&Cerios::Server::ClientServer::onWriteCompleteCallback, this, std::placeholders::_1, std::placeholders::_2, functionLambada));
    }
}

bool Cerios::Server::ClientServer::addClient(std::shared_ptr<Cerios::Server::Client> client) {
    this->clients[client->getSocket()->native_handle()] = client;
    
    MessagePacketHeader header;
    header.id = 0x00;
    header.packetNumber = 0;
    header.payloadLength = static_cast<std::uint32_t>(client->getClientId().size());
    std::vector<std::uint8_t> packetData(sizeof(MessagePacketHeader));
    std::copy(reinterpret_cast<std::uint8_t *>(&header), reinterpret_cast<std::uint8_t *>(&header + sizeof(MessagePacketHeader)), std::back_inserter(packetData));
    std::copy(client->getClientId().begin(), client->getClientId().end(), std::back_inserter(packetData));
    
    this->socket->async_send_to(asio::buffer(packetData), asio::ip::udp::endpoint(asio::ip::address_v4::broadcast() ,this->socket->local_endpoint().port()), std::bind(&Cerios::Server::ClientServer::onWriteComplete, this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

std::weak_ptr<asio::io_service> Cerios::Server::ClientServer::getIOService() {
    return this->owner->getIOService();
}

bool Cerios::Server::ClientServer::onPacketReceived(std::shared_ptr<Cerios::Server::AbstractClient> client, std::shared_ptr<Cerios::Server::Packet> packet) {
    
    
    // We're redirecting the packet to be handled by the client server. Cancel the login/status packet parsing
    // TODO Research TCP Handoff.
    // Most likely only viable on Linux. Might be possible on OS X with porting a BSD driver into a kext.
    return false;
}

void Cerios::Server::ClientServer::clientDisconnected(std::shared_ptr<Cerios::Server::AbstractClient> disconnectedClient) {
    // TODO Gracefully tell node to cleanup client data
    std::cout<<"Client "<<disconnectedClient->getSocket()->remote_endpoint()<<" Disconnected!"<<std::endl;
    if (disconnectedClient->getSocket()->is_open()) {
        disconnectedClient->getSocket()->shutdown(asio::ip::tcp::socket::shutdown_both);
        disconnectedClient->getSocket()->close();
    }
    this->clients.erase(disconnectedClient->getSocket()->native_handle());
}

std::shared_ptr<EVP_PKEY> Cerios::Server::ClientServer::getKeyPair() {
    return owner->getKeyPair();
}

std::shared_ptr<X509> Cerios::Server::ClientServer::getCertificate() {
    return owner->getCertificate();
}