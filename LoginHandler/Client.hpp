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

#include "ClientOwner.hpp"

namespace Cerios { namespace Server {
    class Login;
    class Client : public AbstractClient, public std::enable_shared_from_this<Client> {
    private:
        std::shared_ptr<Cerios::Server::ClientOwner> owner;
        std::array<std::int8_t, 16> verifyToken;
        std::function<int(void)> randomEngine = std::bind(std::uniform_int_distribution<>(0, UINT8_MAX), std::mt19937(std::random_device()()));
        const std::string SessionServer;
    public:
        Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, std::shared_ptr<Cerios::Server::ClientOwner> owner);
        void sendData(std::vector<std::int8_t> &data);
        void onLengthReceive(std::shared_ptr<asio::streambuf>, const asio::error_code &error, std::size_t bytes_transferred);
        void onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred);
        void receivedMessage(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packet);
        void setOwner(std::shared_ptr<Cerios::Server::ClientOwner> newOwner);
        bool authWithMojang();
        Side getSide();
        void disconnect();
    private:
        void startAsyncRead();
    };
}}

#endif /* Client_hpp */
