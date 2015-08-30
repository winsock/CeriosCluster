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

Cerios::Server::ClientServer::ClientServer(std::shared_ptr<asio::ip::tcp::socket> serverEndpoint) : socket(serverEndpoint) {
    this->startAsyncReceive();
}

void Cerios::Server::ClientServer::startAsyncReceive() {
    std::shared_ptr<asio::streambuf> buffer(new asio::streambuf());
    asio::async_read(*this->socket, *buffer, asio::transfer_at_least(1), std::bind(&Cerios::Server::ClientServer::onDataReceived, this, buffer, std::placeholders::_1, std::placeholders::_2));
}

void Cerios::Server::ClientServer::onDataReceived(std::shared_ptr<asio::streambuf>, const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Internal read error from clientserver node: "<<error.message()<<std::endl;
        this->sendShutdownSignal();
    }
}

void Cerios::Server::ClientServer::onWriteCompleteCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<std::function<void(void)>> callback, bool shutdownMessage) {
    if (error && !shutdownMessage) {
        std::cerr<<"Internal read error from clientserver node: "<<error.message()<<std::endl;
        this->sendShutdownSignal();
    }
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
        asio::async_write(*this->socket, asio::buffer(std::string("bye")), std::bind(&Cerios::Server::ClientServer::onWriteCompleteCallback, this, std::placeholders::_1, std::placeholders::_2, functionLambada, true));
    }
}

bool Cerios::Server::ClientServer::addClient(std::shared_ptr<Cerios::Server::Client> client) {
    this->clients[client->getSocket()->native_handle()] = client;
    // TODO Implementation
    return true;
}