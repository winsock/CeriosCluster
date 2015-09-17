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
#include <regex>
#include <chrono>

#include <Packet.hpp>
#include <PingPacket.hpp>
#include <HandshakePacket.hpp>
#include <ServerStatusPacket.hpp>
#include <LoginStartPacket.hpp>
#include <EncryptionPacket.hpp>
#include <LoginSuccessPacket.hpp>
#include <SetCompressionPacket.hpp>
#include <KeepAlivePacket.hpp>

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>

#include <rapidjson/rapidjson.h>

#include "LoginServer.hpp"
#include "hexstring.hpp"
#include "ClientOwner.hpp"

/**
 * HexStr found on stackexchange, should be moved to a helper class/file for the ability to be reused
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

Cerios::Server::Client::Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, std::weak_ptr<Cerios::Server::ClientOwner> owner) : socket(std::move(clientSocket)),owner(owner), SessionServer("/session/minecraft/hasJoined"), keepaliveTimer(*owner.lock()->getIOService().lock(), std::chrono::seconds(4)),
    lastSeen(std::chrono::steady_clock::now()), userid("00000000-0000-0000-0000-000000000000"), writeLock(*owner.lock()->getIOService().lock()) {
    EVP_CIPHER_CTX_init(&this->encryptCipherContext);
    EVP_CIPHER_CTX_init(&this->decryptCipherContext);
    this->startAsyncRead();
}

void Cerios::Server::Client::setOwner(std::weak_ptr<Cerios::Server::ClientOwner> newOwner) {
    this->owner = newOwner;
}

void Cerios::Server::Client::onLengthReceive(std::shared_ptr<asio::streambuf> data, const asio::error_code &error, std::size_t bytes_transferred) {
    if (!alive) {
        return;
    }

    if (error) {
        std::cerr<<"Error during client onLengthReceived: "<<error.message()<<std::endl;
        this->disconnect();
        return;
    }
    
    asio::const_buffer dataBuffer = data->data();
    if (encrypted) {
        std::copy(reinterpret_cast<const uint8_t *>(dataBuffer.data()), reinterpret_cast<const uint8_t *>(dataBuffer.data()) + dataBuffer.size(), std::back_inserter(this->encryptedBuffer));
        std::size_t numberOfBlocksInBuffer = this->encryptedBuffer.size() >> 4;
        std::size_t cyphertextLength = numberOfBlocksInBuffer << 4;
        std::size_t clearBufferOriginalSize = this->buffer.size();
        this->buffer.resize(clearBufferOriginalSize + cyphertextLength);
        std::size_t cleartextLength = this->decrypt(reinterpret_cast<std::uint8_t *>(this->encryptedBuffer.data()), cyphertextLength, reinterpret_cast<std::uint8_t *>(this->buffer.data()) + clearBufferOriginalSize);
        this->buffer.resize(clearBufferOriginalSize + cleartextLength); // Prune any extra memory
    } else {
        std::copy(reinterpret_cast<const uint8_t *>(dataBuffer.data()), reinterpret_cast<const uint8_t *>(dataBuffer.data()) + dataBuffer.size(), std::back_inserter(this->buffer));
    }
    
    std::int32_t messageLength;
    std::size_t varintSize = 0;
    if ((varintSize = Cerios::Server::Packet::readVarIntFromBuffer(&messageLength, this->buffer)) > 0) {
        if (this->buffer.size() >= messageLength + varintSize) {
            // Remove the message length from the buffer
            this->buffer.erase(this->buffer.begin(), this->buffer.begin() + varintSize);
            auto parsedPacket = Cerios::Server::Packet::parsePacket(Cerios::Server::Side::SERVER_BOUND, messageLength, this->buffer, this->state, this->compressionThreshold >= 0);
            if (parsedPacket != nullptr) {
                try {
                    this->receivedMessage(Cerios::Server::Side::SERVER_BOUND, parsedPacket);
                } catch(std::exception &e) {
                    std::cerr<<"Exception during packet handling: "<<e.what()<<std::endl;
                    this->disconnect();
                    return;
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

void Cerios::Server::Client::sendData(std::vector<std::uint8_t> &data) {
    if (!alive) {
        return;
    }
    
    if (encrypted) {
        std::vector<std::uint8_t> encryptedData(data.size() + EVP_MAX_BLOCK_LENGTH); // Plaintext Length in + Cipher blocksize - 1 is the max worse case encrypted data size
        std::size_t actualLength = this->encrypt(data.data(), data.size(), encryptedData.data());
        asio::async_write(*this->socket, asio::buffer(encryptedData, actualLength), asio::bind_executor(this->writeLock, std::bind(&Cerios::Server::Client::onWriteComplete, this, std::placeholders::_1, std::placeholders::_2)));
    } else {
        asio::async_write(*this->socket, asio::buffer(data), asio::bind_executor(this->writeLock, std::bind(&Cerios::Server::Client::onWriteComplete, this, std::placeholders::_1, std::placeholders::_2)));
    }
}

void Cerios::Server::Client::onWriteComplete(const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Error during on send: "<<error.message()<<std::endl;
        this->disconnect();
        return;
    }
}

Cerios::Server::Side Cerios::Server::Client::getSide() {
    return Cerios::Server::Side::SERVER_BOUND;
}

void Cerios::Server::Client::disconnect() {
    if (!alive) {
        return;
    }
    alive = false;
    try {
        this->keepaliveTimer.cancel();
        this->socket->cancel();
        this->socket->shutdown(asio::socket_base::shutdown_both);
        this->socket->close();
    } catch (...) {}
    auto owner = this->owner.lock();
    if (owner != nullptr) {
        owner->clientDisconnected(this);
    }
}

void Cerios::Server::Client::sendPacket(std::shared_ptr<Cerios::Server::Packet> packet) {
    packet->serializePacket(this->getSide());
    if (compressionThreshold >= 0 && std::dynamic_pointer_cast<Cerios::Server::SetCompressionPacket>(packet->shared_from_this()) == nullptr) {
        packet->compressIfLargerThan(this->compressionThreshold);
    }
    packet->framePacket();
    this->sendData(packet->getData());
}

std::string Cerios::Server::Client::getClientId() {
    return this->userid;
}

std::string Cerios::Server::Client::getClientUsername() {
    return this->requestedUsername;
}

void Cerios::Server::Client::formatUUID() {
    if (this->userid.empty()) {
        return;
    }
    
    static std::regex uuidSplit("([[:xdigit:]]{8})([[:xdigit:]]{4})([[:xdigit:]]{4})([[:xdigit:]]{4})([[:xdigit:]]{12})");
    std::smatch uuidParts;
    if (std::regex_match(this->userid, uuidParts, uuidSplit)) {
        this->userid = uuidParts[1].str() + "-" + uuidParts[2].str() + "-" + uuidParts[3].str() + "-" + uuidParts[4].str() + "-" + uuidParts[5].str();
    }
}

void Cerios::Server::Client::receivedMessage(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packet) {
    auto pingPacket = std::dynamic_pointer_cast<Cerios::Server::PingPacket>(packet);
    if (pingPacket != nullptr) {
        // Send it right back
        this->sendPacket(pingPacket);
        if (this->getState() == Cerios::Server::ClientState::STATUS) {
            // If it was a status ping, cancel the pending async operations.
            // Cancel will cause all other waiting operations to error out, thus disconnecting the client after server ping as per protocol spec.
            this->socket->cancel();
        }
        return;
    }
    
    if (std::dynamic_pointer_cast<Cerios::Server::KeepAlivePacket>(packet) != nullptr) {
        lastSeen = std::chrono::steady_clock::now();
        return;
    }
    
    std::cout<<packet->getID()<<std::endl;
    
    auto owner = this->owner.lock();
    if (owner == nullptr) {
        this->disconnect();
        return;
    }
    
    // Test if this client is already authed and should just be forwarding packets.
    if (owner->onPacketReceived(side, this, packet)) {
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
            this->sendPacket(serverStatus);
            return;
        }
    }
    
    if (this->getState() == Cerios::Server::ClientState::LOGIN) {
        // Handle encryption request for login
        auto loginRequest = std::dynamic_pointer_cast<Cerios::Server::LoginStartPacket>(packet);
        if (loginRequest != nullptr) {
            this->requestedUsername = loginRequest->playerName;
            
            if (this->getSocket()->remote_endpoint().address().is_loopback()) {
                std::vector<std::uint8_t> hashData(SHA256_DIGEST_LENGTH);
                SHA256(reinterpret_cast<const std::uint8_t *>(this->requestedUsername.data()), this->requestedUsername.length(), hashData.data());
                this->userid = make_hex_string(hashData.begin(), hashData.end()).substr(0, 32);
                if (userid.length() != 32) {
                    throw new std::runtime_error("WTF Exception, a exception that should never happen has fired.");
                }
                this->formatUUID();
                this->onPlayerLogin();
            } else {
                auto response = Packet::newPacket<Cerios::Server::EncryptionPacket>(Cerios::Server::Side::CLIENT_BOUND, Cerios::Server::ClientState::LOGIN, 0x01);
                
                response->publickKey = owner->getPublicKeyString();
                
                // Generate the verify token
                std::generate_n(this->verifyToken.begin(), this->verifyToken.size(), randomEngine);
                response->clearVerifyToken = this->verifyToken;
                
                this->sendPacket(response);
            }
            return;
        }
        
        // Check if the login request happened first
        if (this->requestedUsername.empty()) {
            return;
        }
        
        // Handle encryption response
        auto encryptionResponse = std::dynamic_pointer_cast<Cerios::Server::EncryptionPacket>(packet);
        if (encryptionResponse != nullptr) {
            EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(owner->getKeyPair().get(), nullptr);
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
            
            if (EVP_PKEY_decrypt(ctx, reinterpret_cast<std::uint8_t *>(verifyTokenBuffer.data()), &outVerifyTokenLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedVerifyToken.data()), encryptionResponse->sealedVerifyToken.size()) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                return;
            }
            if (EVP_PKEY_decrypt(ctx, reinterpret_cast<std::uint8_t *>(encryptionResponse->clearSharedSecret.data()), &outSharedSecretLength, reinterpret_cast<const unsigned char *>(encryptionResponse->sealedSharedSecret.data()), encryptionResponse->sealedSharedSecret.size()) <= 0) {
                EVP_PKEY_CTX_free(ctx);
                return;
            }
            EVP_PKEY_CTX_free(ctx);
            
            verifyTokenBuffer.resize(outVerifyTokenLength);
            encryptionResponse->clearSharedSecret.resize(outSharedSecretLength);
            
            std::copy_n(verifyTokenBuffer.data(), verifyTokenBuffer.size(), reinterpret_cast<std::uint8_t *>(encryptionResponse->clearVerifyToken.data()));
            if (this->verifyToken != encryptionResponse->clearVerifyToken) {
                return;
            }
            
            EVP_EncryptInit_ex(&this->encryptCipherContext, EVP_aes_128_cfb_8(), NULL, encryptionResponse->clearSharedSecret.data(), encryptionResponse->clearSharedSecret.data());
            EVP_CIPHER_CTX_set_padding(&this->encryptCipherContext, false);
            EVP_DecryptInit_ex(&this->decryptCipherContext, EVP_aes_128_cfb_8(), NULL, encryptionResponse->clearSharedSecret.data(), encryptionResponse->clearSharedSecret.data());
            EVP_CIPHER_CTX_set_padding(&this->decryptCipherContext, false);
            
            SHA_CTX *shaContext = new SHA_CTX;
            if (SHA1_Init(shaContext) <= 0) {
                return;
            }
            
            SHA1_Update(shaContext, encryptionResponse->serverId.data(), encryptionResponse->serverId.size());
            SHA1_Update(shaContext, encryptionResponse->clearSharedSecret.data(), encryptionResponse->clearSharedSecret.size());
            
            std::size_t publicKeyLength = i2d_PUBKEY(owner->getKeyPair().get(), NULL);
            unsigned char *tempBuffer, *tempBuffer2;
            tempBuffer = (unsigned char *)malloc(publicKeyLength);
            tempBuffer2 = tempBuffer;
            i2d_PUBKEY(owner->getKeyPair().get(), &tempBuffer2);
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

void Cerios::Server::Client::onHasJoinedPostComplete(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::shared_ptr<asio::streambuf> data, const asio::error_code &error, std::size_t length) {
    if (error) {
        if (error != asio::error::eof) {
            std::cerr<<"Error on ssl received: "<<error.message()<<std::endl;
            return;
        } else if (data->size() < length) {
            return;
        }
    }
    std::string jsonResponseString(reinterpret_cast<const std::uint8_t *>(data->data().data()), reinterpret_cast<const std::uint8_t *>(data->data().data()) + length);
    
    this->playerInfo.Parse(jsonResponseString.c_str());
    this->userid = std::string(this->playerInfo["id"].GetString(), this->playerInfo["id"].GetStringLength());
    this->formatUUID();
    this->requestedUsername = std::string(this->playerInfo["name"].GetString(), this->playerInfo["name"].GetStringLength());
    
    // Set that the connection is encrypted. This is expected before sending the login success packet.
    this->encrypted = true;
    // Clear the buffer of any data. We now expect encrypted data.
    this->buffer.clear();
    std::cout<<"Player: "<<this->requestedUsername<<", id: "<<this->userid<<" enabled encryption successfully!"<<std::endl;
    
    this->onPlayerLogin();
}

void Cerios::Server::Client::onPlayerLogin() {
    // Send Login Success Packet
    auto loginSuccessPacket = Packet::newPacket<Cerios::Server::LoginSuccessPacket>(Cerios::Server::Side::CLIENT_BOUND, Cerios::Server::ClientState::LOGIN, 0x02);
    loginSuccessPacket->uuid = this->userid;
    loginSuccessPacket->username = this->requestedUsername;
    this->sendPacket(loginSuccessPacket);
    
    this->setState(Cerios::Server::ClientState::PLAY);
    // Start the keepalive check
    this->lastSeen = std::chrono::steady_clock::now();
    this->keepaliveTimer.async_wait(std::bind(&Cerios::Server::Client::keepAlive, this, std::placeholders::_1));
    
    // Handoff client to client server relay
    auto owner = this->owner.lock();
    if (owner != nullptr) {
        owner->handoffClient(this);
    }
}

void Cerios::Server::Client::sslHandshakeComplete(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::string request, const asio::error_code &error) {
    if (error) {
        std::cerr<<"Error on ssl handshake: "<<error.message()<<std::endl;
        return;
    }
    asio::async_write(*sslSock, asio::buffer(request), std::bind(&Cerios::Server::Client::sendHTTPRequestDone, this, sslSock, std::placeholders::_1, std::placeholders::_2));
}

void Cerios::Server::Client::sendHTTPRequestDone(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, const asio::error_code &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Error during HTTP Header Read: "<<error.message()<<std::endl;
        return;
    } else {
        std::shared_ptr<asio::streambuf> buffer(new asio::streambuf());
        asio::async_read_until(*sslSock, *buffer, std::string("\r\n\r\n"), std::bind(&Cerios::Server::Client::readHTTPHeader, this, sslSock, buffer, std::placeholders::_1, std::placeholders::_2));
    }
}

void Cerios::Server::Client::readHTTPHeader(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket> > sslSock, std::shared_ptr<asio::streambuf> data, const asio::error_code &error, std::size_t bytes_transferred) {
    if (error || !alive) {
        return;
    }
    
    std::istream dataStream(data.get());
    
    std::map<std::string, std::string> httpKVPairs;
    std::string line;
    std::string::size_type index;
    
    static auto trim = [](std::string const& str) -> std::string {
        if(str.empty())
            return str;
        
        std::size_t firstScan = str.find_first_not_of(' ');
        std::size_t first     = firstScan == std::string::npos ? str.length() : firstScan;
        std::size_t last      = str.find_last_not_of(' ');
        return str.substr(first, last-first+1);
    };
    
    while (std::getline(dataStream, line) && line != "\r") {
        
        index = line.find(':', 0);
        if (index != std::string::npos) {
            httpKVPairs[trim(line.substr(0, index))] = trim(line.substr(index + 1));
        } else if (line.find("HTTP") != std::string::npos) {
            static std::regex httpStatus("(\\S+) (\\d+) ([\\S| ]+)");
            std::smatch matchResults;
            if (std::regex_search(line, matchResults, httpStatus)) {
                if (std::stoi(matchResults.str(2)) != 200) {
                    std::cout<<"Failed to authenticate client: "<<this->socket->remote_endpoint()<<" with requested username: "<<this->requestedUsername<<std::endl;
                    this->socket->cancel(); // Disconnect Client
                    return;
                }
            }
        }
    }
    try {
        std::size_t contentLength = std::stoul(httpKVPairs["Content-Length"]);
        asio::async_read(*sslSock, *data, asio::transfer_exactly(contentLength), std::bind(&Cerios::Server::Client::onHasJoinedPostComplete, this, sslSock, data, std::placeholders::_1, contentLength));
    } catch (...) {
        std::cerr<<"Error parsing Content Length from Mojang!"<<std::endl;
    }
}

void Cerios::Server::Client::connectedToMojang(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::string request, const asio::error_code &error) {
    sslSock->lowest_layer().set_option(asio::ip::tcp::no_delay(true));
    /**
     * TODO: Embed the Root cert that signs Mojang's SSL Cert so we can actually verify the session server endpoint.
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
    auto owner = this->owner.lock();
    if (owner == nullptr) {
        return;
    }
    auto ioService = owner->getIOService().lock();
    if (ioService == nullptr) {
        return;
    }
    
    std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sock(new asio::ssl::stream<asio::ip::tcp::socket>(*ioService, ctx));
    asio::ip::tcp::resolver resolver(*ioService);
    asio::ip::tcp::resolver::query query("sessionserver.mojang.com", "https");
    asio::async_connect(sock->lowest_layer(), resolver.resolve(query), std::bind(&Cerios::Server::Client::connectedToMojang, this, sock, std::move(httpHeader.str()), std::placeholders::_1));
}

void Cerios::Server::Client::keepAlive(const asio::error_code &error) {
    if (!alive) {
        return;
    }
    if (error) {
        this->disconnect();
        return;
    }
    
    if ((std::chrono::steady_clock::now() - this->lastSeen) > std::chrono::seconds(10)) {
        // Timeout
        this->socket->cancel();
    } else {
        auto keepAlive = Cerios::Server::Packet::newPacket<KeepAlivePacket>(Cerios::Server::Side::CLIENT_BOUND, this->getState(), 0x00);
        if (keepAlive == nullptr) {
            // Not in play game state
            this->socket->cancel();
            return;
        }
        this->sendPacket(keepAlive);
        // Reset the timer
        this->keepaliveTimer.expires_from_now(std::chrono::seconds(4));
        this->keepaliveTimer.async_wait(std::bind(&Cerios::Server::Client::keepAlive, this, std::placeholders::_1));
    }
}

int Cerios::Server::Client::encrypt(unsigned char *plaintext, std::size_t plaintext_len, unsigned char *ciphertext) {
    int len;
    int ciphertext_len;
    
    /* Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(&this->encryptCipherContext, ciphertext, &len, plaintext, static_cast<int>(plaintext_len))) {
        return -1;
    }
    ciphertext_len = len;
    
    return ciphertext_len;
}

int Cerios::Server::Client::decrypt(unsigned char *ciphertext, std::size_t ciphertext_len, unsigned char *plaintext) {
    int len;
    int plaintext_len;
    
    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_DecryptUpdate(&this->decryptCipherContext, plaintext, &len, ciphertext, static_cast<int>(ciphertext_len))) {
        return -1;
    }
    plaintext_len = len;
    
    return plaintext_len;
}