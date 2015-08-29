//
//  Client.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/28/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "Client.hpp"
#include <asio.hpp>
#include <iostream>
#include <istream>

#include "Packet.hpp"
#include "HandshakePacket.hpp"
#include "LoginServer.hpp"

Cerios::Server::Client::Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, Cerios::Server::Login *parentPtr) : socket(clientSocket), buffer(new std::vector<std::int8_t>()), parent(parentPtr) {
    this->startAsyncRead();
}

void Cerios::Server::Client::onLengthReceive(std::shared_ptr<asio::streambuf> data, const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Error during onLengthReceived: "<<error.message()<<std::endl;
        this->parent->clientDisconnected(this);
        return;
    }
    
    data->commit(bytes_transferred);
    asio::const_buffer dataBuffer = data->data();
    std::copy(reinterpret_cast<const uint8_t *>(dataBuffer.data()), &reinterpret_cast<const uint8_t *>(dataBuffer.data())[dataBuffer.size()], std::back_inserter(*this->buffer));
    
    try {
        std::int32_t messageLength;
        std::size_t varintSize;
        if ((varintSize = Cerios::Server::Packet::readVarIntFromBuffer(&messageLength, this->buffer.get()))) {
            // Remove the message length from the buffer
            this->buffer->erase(this->buffer->begin(), this->buffer->begin() + varintSize);
            if (dataBuffer.size() >= messageLength + varintSize) {
                auto parsedPacket = Cerios::Server::Packet::parsePacket(messageLength, this->buffer, this->state);
                parsedPacket->onReceivedBy(this);
            }
        }
    } catch(std::exception &e) {
        std::cerr<<"Exception during reading varint: "<<e.what()<<std::endl;
    }
    this->startAsyncRead();
}

void Cerios::Server::Client::startAsyncRead() {
    std::shared_ptr<asio::streambuf> buffer(new asio::streambuf());
    asio::async_read(*this->socket, *buffer, asio::transfer_at_least(1), std::bind(&Cerios::Server::Client::onLengthReceive, this, buffer, std::placeholders::_1, std::placeholders::_2));
}

void Cerios::Server::Client::sendData(std::vector<std::int8_t> &data) {
    asio::async_write(*this->socket, asio::buffer(data), std::bind(&Cerios::Server::Client::onWriteComplete, this, std::placeholders::_1, std::placeholders::_2));
}

void Cerios::Server::Client::onWriteComplete(const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Error during on send: "<<error.message()<<std::endl;
        this->parent->clientDisconnected(this);
        return;
    }
}

void Cerios::Server::Client::setState(ClientState state) {
    this->state = state;
}

Cerios::Server::ClientState Cerios::Server::Client::getState() {
    return this->state;
}

std::shared_ptr<asio::ip::tcp::socket> Cerios::Server::Client::getSocket() {
    return this->socket;
}