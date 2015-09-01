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
#include <random>

#include <Packet.hpp>
#include <PingPacket.hpp>
#include <HandshakePacket.hpp>
#include <ServerStatusPacket.hpp>
#include <LoginStartPacket.hpp>
#include <EncryptionPacket.hpp>

#include "LoginServer.hpp"

Cerios::Server::Client::Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, std::shared_ptr<Cerios::Server::ClientOwner> owner) : AbstractClient(clientSocket), owner(owner) {
    this->startAsyncRead();
}

void Cerios::Server::Client::setOwner(std::shared_ptr<Cerios::Server::ClientOwner> newOwner) {
    this->owner = newOwner;
}

void Cerios::Server::Client::onLengthReceive(std::shared_ptr<asio::streambuf> data, const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Error during onLengthReceived: "<<error.message()<<std::endl;
        this->disconnect();
        return;
    }
    
    asio::const_buffer dataBuffer = data->data();
    std::copy(reinterpret_cast<const uint8_t *>(dataBuffer.data()), &reinterpret_cast<const uint8_t *>(dataBuffer.data())[dataBuffer.size()], std::back_inserter(*this->buffer));
    try {
        std::int32_t messageLength;
        std::size_t varintSize = 0;
        while ((varintSize = Cerios::Server::Packet::readVarIntFromBuffer(&messageLength, this->buffer.get()))) {
            if (this->buffer->size() >= messageLength + varintSize) {
                // Remove the message length from the buffer
                this->buffer->erase(this->buffer->begin(), this->buffer->begin() + varintSize);
                auto parsedPacket = Cerios::Server::Packet::parsePacket(Cerios::Server::Side::CLIENT, messageLength, this->buffer, this->state);
                if (parsedPacket != nullptr) {
                    this->receivedMessage(Cerios::Server::Side::CLIENT, parsedPacket);
                }
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
        this->disconnect();
        return;
    }
}

Cerios::Server::Side Cerios::Server::Client::getSide() {
    return Cerios::Server::Side::SERVER;
}

void Cerios::Server::Client::disconnect() {
    this->owner->clientDisconnected(this->shared_from_this());
}

void Cerios::Server::Client::receivedMessage(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packet) {
    // This is the server, I hope we don't receive messages from ourselves....
    if (side != Side::CLIENT) {
        return;
    }
    
    auto pingPacket = std::dynamic_pointer_cast<Cerios::Server::PingPacket>(packet);
    if (pingPacket != nullptr) {
        // Send it right back
        pingPacket->sendTo(this);
        this->setState(ClientState::HANDSHAKE);
        return;
    }
    
    // Test if this client is already authed and should just be forwarding packets.
    if (!this->owner->onPacketReceived(side, this->shared_from_this(), packet)) {
        return;
    }
    
    // Handle handshake requests if we're in handshake state
    if (this->getState() == Cerios::Server::ClientState::HANDSHAKE) {
        auto handshake = std::dynamic_pointer_cast<Cerios::Server::HandshakePacket>(packet);
        if (handshake != nullptr) {
            this->setState(handshake->requestedNextState);
            return;
        }
    }
    
    // Handle status requests
    if (this->getState() == Cerios::Server::ClientState::STATUS) {
        auto serverStatus = std::dynamic_pointer_cast<Cerios::Server::ServerStatusPacket>(packet);
        if (serverStatus != nullptr) {
            serverStatus->jsonEncodedServerStatus = "{"
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
            serverStatus->sendTo(this);
            return;
        }
    }
    
    if (this->getState() == Cerios::Server::ClientState::LOGIN) {
        // Handle encryption request for login
        auto loginRequest = std::dynamic_pointer_cast<Cerios::Server::LoginStartPacket>(packet);
        if (loginRequest != nullptr) {
            // TODO locahost/::1 check to disable encryption
            auto response = std::dynamic_pointer_cast<Cerios::Server::EncryptionPacket>(Packet::newPacket(Cerios::Server::Side::SERVER, Cerios::Server::ClientState::LOGIN, 0x1));
            if (this->owner->getCertificate() == nullptr) {
                return;
            }
            response->keyPair = this->owner->getKeyPair();
            
            // Generate the verify token
            std::generate_n(this->verifyToken.begin(), this->verifyToken.size(), randomEngine);
            response->clearVerifyToken = this->verifyToken;
            
            response->sendTo(this);
            return;
        }

        // Handle encryption response
        auto encryptionResponse = std::dynamic_pointer_cast<Cerios::Server::EncryptionPacket>(packet);
        if (encryptionResponse != nullptr) {
            if (this->owner->getKeyPair() == nullptr) {
                return;
            }
            EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(this->owner->getKeyPair().get(), nullptr);
            if (!ctx) {
                return;
            }
            if (EVP_PKEY_decrypt_init(ctx) <= 0) {
                return;
            }
            /* Error */
            if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
                return;
            }
            
            std::size_t outVerifyTokenLength, outSharedSecretLength;
            EVP_PKEY_decrypt(ctx, nullptr, &outVerifyTokenLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedVerifyToken.data()), encryptionResponse->sealedVerifyToken.size());
            EVP_PKEY_decrypt(ctx, nullptr, &outSharedSecretLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedSharedSecret.data()), encryptionResponse->sealedSharedSecret.size());
            
            std::vector<std::uint8_t> verifyTokenBuffer(outVerifyTokenLength); // Worst case size
            encryptionResponse->clearSharedSecret.resize(outSharedSecretLength); // Worst case
            
            if (EVP_PKEY_decrypt(ctx, reinterpret_cast<unsigned char *>(verifyTokenBuffer.data()), &outVerifyTokenLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedVerifyToken.data()), encryptionResponse->sealedSharedSecret.size()) <= 0) {
                return;
            }
            if (EVP_PKEY_decrypt(ctx, reinterpret_cast<unsigned char *>(encryptionResponse->clearSharedSecret.data()), &outVerifyTokenLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedSharedSecret.data()), encryptionResponse->sealedSharedSecret.size()) <= 0) {
                return;
            }
            
            EVP_PKEY_CTX_free(ctx);
            
            if (!std::equal(this->verifyToken.begin(), this->verifyToken.end(), verifyTokenBuffer.begin())) {
                return;
            }

            // TODO ENABLE ENCRYPTION
            std::cout<<"Client: "<<this->getSocket()->remote_endpoint()<<" enabled encryption successfully!"<<std::endl;
            return;
        }
    }
}