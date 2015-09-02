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

#include <AbstractClient.hpp>
#include <rapidjson/document.h>

#include "ClientOwner.hpp"

namespace Cerios { namespace Server {
    class Login;
    class Client : public AbstractClient, public std::enable_shared_from_this<Client> {
    private:
        std::shared_ptr<Cerios::Server::ClientOwner> owner;
        std::array<std::int8_t, 16> verifyToken;
        std::shared_ptr<std::vector<std::int8_t>> httpBuffer;

        EVP_CIPHER_CTX encryptCipherContext;
        EVP_CIPHER_CTX decryptCipherContext;
        bool encrypted = false;
        std::shared_ptr<std::vector<std::int8_t>> encryptedBuffer;

        std::function<int(void)> randomEngine = std::bind(std::uniform_int_distribution<>(0, UINT8_MAX), std::mt19937(std::random_device()()));
        const std::string SessionServer;
        const std::string httpNewline = "\r\n";
        const std::string dataSeparator = "\r\n\r\n";
        const std::string contentLengthField = "Content-Length: ";
        std::string requestedUsername, userid;
        rapidjson::Document playerInfo;
    public:
        Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, std::shared_ptr<Cerios::Server::ClientOwner> owner);
        void sendData(std::vector<std::int8_t> &data);
        void onLengthReceive(std::shared_ptr<asio::streambuf>, const asio::error_code &error, std::size_t bytes_transferred);
        void onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred);
        void onHasJoinedPostComplete(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::shared_ptr<asio::streambuf> data, std::uint64_t contentLength, const asio::error_code& error, std::size_t bytes_transferred);
        void receivedMessage(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packet);
        void setOwner(std::shared_ptr<Cerios::Server::ClientOwner> newOwner);
        void authWithMojang(std::string serverIdHexDigest);
        void connectedToMojang(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::string request, const asio::error_code &error);
        void sslHandshakeComplete(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> sslSock, std::string request, const asio::error_code &error);
        Side getSide();
        void disconnect();
    private:
        int encrypt(unsigned char *plaintext, std::size_t plaintext_len, unsigned char *ciphertext);
        int decrypt(unsigned char *ciphertext, std::size_t ciphertext_len, unsigned char *plaintext);
        void startAsyncRead();
    };
}}

#endif /* Client_hpp */
