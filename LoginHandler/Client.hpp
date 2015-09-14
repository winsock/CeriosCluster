//
//  Client.hpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/28/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef Client_hpp
#define Client_hpp

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <array>
#include <string>
#include <random>
#include <chrono>

#include <rapidjson/document.h>

#include "ClientOwner.hpp"

namespace Cerios { namespace Server {
    class Login;
    class Client {
    private:
        std::weak_ptr<Cerios::Server::ClientOwner> owner;
        std::shared_ptr<asio::ip::tcp::socket> socket;
        std::vector<std::uint8_t> buffer;
        ClientState state = ClientState::HANDSHAKE;
        
        // 256 is the same value that Mojang sets as their default compression threshold size.
        const std::size_t defaultCompressionThreshold = 256;
        // Start with no compression
        std::int32_t compressionThreshold = -1;

        EVP_CIPHER_CTX encryptCipherContext;
        EVP_CIPHER_CTX decryptCipherContext;
        bool encrypted = false;
        std::vector<std::uint8_t> encryptedBuffer;

        std::function<int(void)> randomEngine = std::bind(std::uniform_int_distribution<>(0, UINT8_MAX), std::mt19937(std::random_device()()));
        std::array<std::uint8_t, 4> verifyToken;
        const std::string SessionServer;
        std::string requestedUsername, userid;
        rapidjson::Document playerInfo;
        
        asio::steady_timer keepaliveTimer;
        std::chrono::time_point<std::chrono::steady_clock> lastSeen;
        
        asio::io_service::strand writeLock;
        
        bool alive = true;
    public:
        Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, std::weak_ptr<Cerios::Server::ClientOwner> owner);
        void sendData(std::vector<std::uint8_t> &data);
        void sendPacket(std::shared_ptr<Cerios::Server::Packet> packet);
        void onLengthReceive(std::shared_ptr<asio::streambuf>, const asio::error_code &error, std::size_t bytes_transferred);
        void onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred);
        void onHasJoinedPostComplete(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::shared_ptr<asio::streambuf> data, const asio::error_code& error, std::size_t bytes_transferred);
        void receivedMessage(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packet);
        void setOwner(std::weak_ptr<Cerios::Server::ClientOwner> newOwner);
        void authWithMojang(std::string serverIdHexDigest);
        void connectedToMojang(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::string request, const asio::error_code &error);
        void sslHandshakeComplete(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::string request, const asio::error_code &error);
        void sendHTTPRequestDone(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, const asio::error_code &error, std::size_t bytes_transferred);
        void readHTTPHeader(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::shared_ptr<asio::streambuf> data, const asio::error_code &error, std::size_t bytes_transferred);
        Side getSide();
        ClientState getState() { return state; }
        std::size_t getCompressionThreshold() { return compressionThreshold; }
        void setState(ClientState state) { this->state = state; }
        asio::ip::tcp::socket *getSocket() { return socket.get(); }
        std::string getClientId();
        void disconnect();
    private:
        int encrypt(unsigned char *plaintext, std::size_t plaintext_len, unsigned char *ciphertext);
        int decrypt(unsigned char *ciphertext, std::size_t ciphertext_len, unsigned char *plaintext);
        void startAsyncRead();
        void keepAlive(const asio::error_code &error);
        void onPlayerLogin();
    };
}}

#endif /* Client_hpp */
