//
//  Client.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/28/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "Client.hpp"

#include <iostream>
#include <istream>
#include <random>
#include <sstream>

#include <Packet.hpp>
#include <PingPacket.hpp>
#include <HandshakePacket.hpp>
#include <ServerStatusPacket.hpp>
#include <LoginStartPacket.hpp>
#include <EncryptionPacket.hpp>
#include <openssl/crypto.h>

#include "LoginServer.hpp"
#include "ClientOwner.hpp"

/**
 * HexStr found on stackexchange, shouble be moved to a helper class/file for the ability to be reused
 * http://codereview.stackexchange.com/questions/78535/converting-array-of-bytes-to-the-hex-string-representation
 **/
constexpr char hexmap[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

std::string hexStr(unsigned char *data, int len) {
    std::string s(len * 2, ' ');
    for (int i = 0; i < len; ++i) {
        s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
        s[2 * i + 1] = hexmap[data[i] & 0x0F];
    }
    return s;
}

Cerios::Server::Client::Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, std::shared_ptr<Cerios::Server::ClientOwner> owner) : AbstractClient(clientSocket), owner(owner), SessionServer("/session/minecraft/hasJoined"), httpBuffer(new std::vector<std::int8_t>) {
    this->startAsyncRead();
}

void Cerios::Server::Client::setOwner(std::shared_ptr<Cerios::Server::ClientOwner> newOwner) {
    this->owner = newOwner;
}

void Cerios::Server::Client::onLengthReceive(std::shared_ptr<asio::streambuf> data, const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Error during onLengthReceived: "<<error.message()<<std::endl;
        if (error != asio::error::operation_aborted) {
            this->disconnect();
        }
        return;
    }
    
    asio::const_buffer dataBuffer = data->data();
    std::copy(reinterpret_cast<const uint8_t *>(dataBuffer.data()), &reinterpret_cast<const uint8_t *>(dataBuffer.data())[dataBuffer.size()], std::back_inserter(*this->buffer));
    
    std::int32_t messageLength;
    std::size_t varintSize = 0;
    while ((varintSize = Cerios::Server::Packet::readVarIntFromBuffer(&messageLength, this->buffer.get()))) {
        if (this->buffer->size() >= messageLength + varintSize) {
            // Remove the message length from the buffer
            this->buffer->erase(this->buffer->begin(), this->buffer->begin() + varintSize);
            auto parsedPacket = Cerios::Server::Packet::parsePacket(Cerios::Server::Side::CLIENT, messageLength, this->buffer, this->state);
            if (parsedPacket != nullptr) {
                try {
                    this->receivedMessage(Cerios::Server::Side::CLIENT, parsedPacket);
                } catch(std::exception &e) {
                    std::cerr<<"Exception during packet handling: "<<e.what()<<std::endl;
                }
            }
        }
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
            this->requestedUsername = loginRequest->playerName;
            
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

        // Check if the login request happened first
        if (this->requestedUsername.empty()) {
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
                EVP_PKEY_CTX_free(ctx);
                return;
            }
            if (EVP_PKEY_decrypt_init(ctx) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                return;
            }
            /* Error */
            if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                return;
            }
            
            std::size_t outVerifyTokenLength, outSharedSecretLength;
            EVP_PKEY_decrypt(ctx, nullptr, &outVerifyTokenLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedVerifyToken.data()), encryptionResponse->sealedVerifyToken.size());
            EVP_PKEY_decrypt(ctx, nullptr, &outSharedSecretLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedSharedSecret.data()), encryptionResponse->sealedSharedSecret.size());
            
            std::vector<std::uint8_t> verifyTokenBuffer(outVerifyTokenLength); // Worst case size
            encryptionResponse->clearSharedSecret.resize(outSharedSecretLength); // Worst case
            
            if (EVP_PKEY_decrypt(ctx, reinterpret_cast<unsigned char *>(verifyTokenBuffer.data()), &outVerifyTokenLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedVerifyToken.data()), encryptionResponse->sealedVerifyToken.size()) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                return;
            }
            if (EVP_PKEY_decrypt(ctx, reinterpret_cast<unsigned char *>(encryptionResponse->clearSharedSecret.data()), &outSharedSecretLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedSharedSecret.data()), encryptionResponse->sealedSharedSecret.size()) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                return;
            }
            EVP_PKEY_CTX_free(ctx);
            
            verifyTokenBuffer.resize(outVerifyTokenLength);
            encryptionResponse->clearSharedSecret.resize(outSharedSecretLength);
            
            std::copy_n(verifyTokenBuffer.begin(), verifyTokenBuffer.size(), encryptionResponse->clearVerifyToken.data());
            if (this->verifyToken != encryptionResponse->clearVerifyToken) {
                return;
            }
            
            SHA_CTX *shaContext = new SHA_CTX;
            if (SHA1_Init(shaContext) <= 0) {
                return;
            }
            
            SHA1_Update(shaContext, encryptionResponse->serverId.data(), encryptionResponse->serverId.size());
            SHA1_Update(shaContext, encryptionResponse->clearSharedSecret.data(), encryptionResponse->clearSharedSecret.size());
            
            std::size_t publicKeyLength = i2d_PUBKEY(this->owner->getKeyPair().get(), NULL);
            unsigned char *tempBuffer, *tempBuffer2;
            tempBuffer = (unsigned char *)malloc(publicKeyLength);
            tempBuffer2 = tempBuffer;
            i2d_PUBKEY(this->owner->getKeyPair().get(), &tempBuffer2);
            SHA1_Update(shaContext, tempBuffer, publicKeyLength);
            free(tempBuffer);
            
            std::array<std::uint8_t, SHA_DIGEST_LENGTH> shaDigest;
            SHA1_Final(shaDigest.data(), shaContext);
            delete shaContext;

            bool negitiveHash = false;
            if ((shaDigest[0] & 0x80) == 0x80) {
                // Negitive hash, make it twos complement
                negitiveHash = true;
                bool carry = true;
                for (int i = shaDigest.size() - 1; i >= 0; i--) {
                    shaDigest[i] = ~shaDigest[i];
                    if (carry) {
                        carry = shaDigest[i] == 0xFF;
                        shaDigest[i]++;
                    }
                }
            }
            
            /**
             *  Note that the Sha1.hexdigest() method used by minecraft removes leading zeros and uses the two's-complement of negative numbers prefixed with a minus sign
             **/
            std::string hashString = hexStr(shaDigest.data(), shaDigest.size());
            while (hashString.front() == '0') {
                hashString.erase(hashString.begin());
            }
            if (negitiveHash) {
                hashString.insert(hashString.begin(), '-');
            }
            
            this->authWithMojang(hashString);
            
            return;
        }
    }
}

void Cerios::Server::Client::onHasJoinedPostComplete(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::shared_ptr<asio::streambuf> data, std::uint64_t contentLength, const asio::error_code &error, std::size_t bytes_transferred) {
    std::cout<<bytes_transferred<<std::endl;
    if (error && error != asio::error::eof) {
        std::cerr<<"Error on ssl received: "<<error.message()<<std::endl;
        return;
    }
    try {
        asio::const_buffer dataBuffer = data->data();
        std::copy(reinterpret_cast<const uint8_t *>(dataBuffer.data()), &reinterpret_cast<const uint8_t *>(dataBuffer.data())[dataBuffer.size()], std::back_inserter(*this->httpBuffer));
        auto dataLocation = std::search(this->httpBuffer->begin(), this->httpBuffer->end(), this->dataSeparator.begin(), this->dataSeparator.end());
        if (dataLocation != this->httpBuffer->end()) {
            if (contentLength <= 0) {
                auto contentLengthLocation = std::search(this->httpBuffer->begin(), this->httpBuffer->end(), this->contentLengthField.begin(), this->contentLengthField.end());
                auto contentLengthLineEnd = std::search(contentLengthLocation, this->httpBuffer->end(), this->httpNewline.begin(), this->httpNewline.end());
                contentLength = std::stoul(std::string(contentLengthLocation + this->contentLengthField.size(), contentLengthLineEnd));
                if (contentLength <= 0) {
                    return;
                }
            }
            if (this->httpBuffer->end() - (dataLocation + this->dataSeparator.size()) >= contentLength) {
                // Full content
                std::string jsonResponseString(dataLocation + this->dataSeparator.size(), dataLocation + this->dataSeparator.size() + contentLength);
                std::cout<<"Got response back from sessionserver!"<<std::endl<<jsonResponseString<<std::endl;
                // TODO ENABLE ENCRYPTION
                std::cout<<"Client: "<<this->getSocket()->remote_endpoint()<<" enabled encryption successfully!"<<std::endl;
                httpBuffer->clear();
            }
        }
        if (error != asio::error::eof) {
            std::shared_ptr<asio::streambuf> buffer(new asio::streambuf());
            asio::async_read(*sslSock, *buffer, std::bind(&Cerios::Server::Client::onHasJoinedPostComplete, this, sslSock, buffer, contentLength, std::placeholders::_1, std::placeholders::_2));
        }
    } catch (std::exception e) {
        std::cerr<<"Error during on sslRead: "<<e.what()<<std::endl;
    }
}

void Cerios::Server::Client::sslHandshakeComplete(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket> > sslSock, std::string request, const asio::error_code &error) {
    if (error) {
        std::cerr<<"Error on ssl handshake: "<<error.message()<<std::endl;
        return;
    }
    asio::async_write(*sslSock, asio::buffer(request), std::bind([](std::shared_ptr<Cerios::Server::Client> client, std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket> > sslSock, const asio::error_code &error, std::size_t bytes_transferred) -> void {
        if (error) {
            std::cerr<<"Error during on session send: "<<error.message()<<std::endl;
            return;
        } else {
            std::shared_ptr<asio::streambuf> buffer(new asio::streambuf());
            asio::async_read(*sslSock, *buffer, std::bind(&Cerios::Server::Client::onHasJoinedPostComplete, client.get(), sslSock, buffer, 0, std::placeholders::_1, std::placeholders::_2));
        }
    }, this->shared_from_this(), sslSock, std::placeholders::_1, std::placeholders::_2));
}

void Cerios::Server::Client::connectedToMojang(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::string request, const asio::error_code &error) {
    sslSock->lowest_layer().set_option(asio::ip::tcp::no_delay(true));
    /**
     * TODO: Embed the Root cert that signs Mojang's SSL Cert so we can actually verify.
     **/
    // Perform SSL handshake and verify the remote host's certificate.
    /* sock->set_verify_mode(asio::ssl::verify_peer);
     sock->set_verify_callback(asio::ssl::rfc2818_verification("sessionserver.mojang.com")); */
    sslSock->async_handshake(asio::ssl::stream_base::client, std::bind(&Cerios::Server::Client::sslHandshakeComplete, this, sslSock, request, std::placeholders::_1));
}

void Cerios::Server::Client::authWithMojang(std::string serverIdHexDigest) {
    std::stringstream httpHeader;
    httpHeader<<"GET "<<this->SessionServer<<"?username="<<this->requestedUsername<<"&serverId="<<serverIdHexDigest<<" HTTP/1.1"<<"\r\n";
    httpHeader<<"Host: "<<"sessionserver.mojang.com"<<"\r\n";
    httpHeader<<"User-Agent: "<<"Cerios Cluster (Minecraft Server)"<<"\r\n";
    httpHeader<<"Connection: "<<"Close"<<"\r\n\r\n";
    
    // Create a context that uses the default paths for finding CA certificates.
    asio::ssl::context ctx(asio::ssl::context::sslv23);
    ctx.set_default_verify_paths();
    
    // Open a socket and connect it to the remote host.
    std::shared_ptr<asio::io_service> service = this->owner->getIOService().lock();
    if (service == nullptr) {
        std::cerr<<"Error getting lock on IO Service"<<std::endl;
        return;
    }
    std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sock(new asio::ssl::stream<asio::ip::tcp::socket>(*service, ctx));
    asio::ip::tcp::resolver resolver(*service);
    asio::ip::tcp::resolver::query query("sessionserver.mojang.com", "https");
    asio::async_connect(sock->lowest_layer(), resolver.resolve(query), std::bind(&Cerios::Server::Client::connectedToMojang, this, sock, httpHeader.str(), std::placeholders::_1));
}