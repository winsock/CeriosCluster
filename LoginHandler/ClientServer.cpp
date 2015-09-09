//
//  ClientServer.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/30/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "ClientServer.hpp"

#include <iostream>
#include <Packet.hpp>

#include "Client.hpp"
#include "LoginServer.hpp"

Cerios::Server::ClientServer::ClientServer(std::uint16_t messageReceive, bool ipv6, std::shared_ptr<Cerios::Server::Login> owner) :
sendSocket(new asio::ip::udp::socket(*owner->getIOService().lock())),
receiveSocket(new asio::ip::udp::socket(*owner->getIOService().lock())), owner(owner) {
    // Enable broadcast
    sendSocket->open(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4());
    sendSocket->set_option(asio::socket_base::reuse_address(true));
    sendSocket->set_option(asio::socket_base::broadcast(true));
    sendSocket->bind(asio::ip::udp::endpoint(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4(), 0));
    
    receiveSocket->open(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4());
    receiveSocket->set_option(asio::socket_base::reuse_address(true));
    receiveSocket->bind(asio::ip::udp::endpoint(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4(), messageReceive));
    
    this->startAsyncReceive();
}

void Cerios::Server::ClientServer::startAsyncReceive() {
    this->receiveSocket->async_wait(asio::socket_base::wait_read, std::bind(&Cerios::Server::ClientServer::onDataReceived, this, std::placeholders::_1));
}

void Cerios::Server::ClientServer::onDataReceived(const asio::error_code &error) {
    if (error) {
        std::cerr<<error.message()<<std::endl;
        return;
    }
    
    std::size_t bytesAvailable = this->receiveSocket->available();
    std::vector<std::uint8_t> data(bytesAvailable);
    
    asio::ip::udp::endpoint senderEndpoint;
    asio::error_code receiveError;
    std::size_t packetSize = this->receiveSocket->receive_from(asio::buffer(data, bytesAvailable), senderEndpoint, 0, receiveError);
    if (receiveError) {
        std::cerr<<receiveError.message()<<std::endl;
        return;
    }
    
    data.resize(packetSize);
    this->handleMessage(senderEndpoint, Cerios::InternalComms::Packet::fromData(data));
    
    this->startAsyncReceive();
}

void Cerios::Server::ClientServer::handleMessage(asio::ip::udp::endpoint &endpoint, std::shared_ptr<Cerios::InternalComms::Packet> packet) {
    switch (packet->getMessageID()) {
        case Cerios::InternalComms::MessageID::ACK:
            std::cout<<"Client Accepted"<<std::endl;
            break;
        default:
            break;
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
    for (auto &clientMapPair : this->clients) {
        clientMapPair.second->disconnect();
    }
    if (this->receiveSocket->is_open()) {
        std::shared_ptr<std::function<void(std::shared_ptr<asio::ip::udp::socket> socket)>> functionLambada(new std::function<void(std::shared_ptr<asio::ip::udp::socket> socket)>([](std::shared_ptr<asio::ip::udp::socket> socket) -> void{
            if (socket->is_open()) {
                socket->close();
            }
        }));
        (*functionLambada)(this->sendSocket);
        (*functionLambada)(this->receiveSocket);
//        asio::async_write(*this->socket, asio::buffer(std::string("bye")), std::bind(&Cerios::Server::ClientServer::onWriteCompleteCallback, this, std::placeholders::_1, std::placeholders::_2, functionLambada));
    }
}

bool Cerios::Server::ClientServer::addClient(std::unique_ptr<Cerios::Server::Client> client) {
    std::vector<std::uint8_t> payload;
    std::copy(client->getClientId().begin(), client->getClientId().end(), std::back_inserter(payload));
    std::shared_ptr<Cerios::InternalComms::Packet> message = Cerios::InternalComms::Packet::newPacket(Cerios::InternalComms::MessageID::ACCEPT_CLIENT, client->getClientId(), payload);
    this->clients[client->getSocket()->native_handle()] = std::move(client);

    std::vector<std::uint8_t> rawData;
    message->serializeData(rawData);
    this->sendSocket->async_send_to(asio::buffer(rawData), asio::ip::udp::endpoint(asio::ip::address_v4::broadcast(), 1337), std::bind(&Cerios::Server::ClientServer::onWriteComplete, this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

std::weak_ptr<asio::io_service> Cerios::Server::ClientServer::getIOService() {
    return this->owner->getIOService();
}

/** 
 * TODO Research TCP Handoff.
 * We're redirecting the packet to be handled by the client server. Cancel the login/status packet parsing
 * Most likely only viable on Linux. Might be possible on OS X with porting a BSD driver into a kext.
**/
bool Cerios::Server::ClientServer::onPacketReceived(Cerios::Server::AbstractClient *abstractClient, std::shared_ptr<Cerios::Server::Packet> packet) {
    Cerios::Server::Client *client = dynamic_cast<Cerios::Server::Client *>(abstractClient);
    if (client == nullptr) {
        return false;
    }
    
    std::vector<std::uint8_t> payload;
    packet->serializeToBuffer(Cerios::Server::Side::CLIENT, payload);
    std::shared_ptr<Cerios::InternalComms::Packet> message = Cerios::InternalComms::Packet::newPacket(Cerios::InternalComms::MessageID::MC_PACKET, client->getClientId(), payload);
    
    std::vector<std::uint8_t> messageData;
    message->serializeData(messageData);
    
    this->sendSocket->async_send_to(asio::buffer(messageData), asio::ip::udp::endpoint(asio::ip::address_v4::broadcast(), 1337), std::bind(&Cerios::Server::ClientServer::onWriteComplete, this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

void Cerios::Server::ClientServer::clientDisconnected(Cerios::Server::AbstractClient *disconnectedClient) {
    // TODO Gracefully tell node to cleanup client data
    try {
        std::cout<<"Client "<<disconnectedClient->getSocket()->remote_endpoint()<<" Disconnected!"<<std::endl;
        disconnectedClient->getSocket()->cancel();

        if (disconnectedClient->getSocket()->is_open()) {
            disconnectedClient->getSocket()->close();
        }
    } catch (...) {}
    this->clients.erase(disconnectedClient->getSocket()->native_handle());
}

std::shared_ptr<EVP_PKEY> Cerios::Server::ClientServer::getKeyPair() {
    return owner->getKeyPair();
}

std::shared_ptr<X509> Cerios::Server::ClientServer::getCertificate() {
    return owner->getCertificate();
}