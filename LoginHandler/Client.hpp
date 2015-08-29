//
//  Client.hpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/28/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef Client_hpp
#define Client_hpp
#include <asio.hpp>
#include <asio/read.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>

#include "Packet.hpp"

namespace Cerios { namespace Server {
    class Login;
    class Client {
    private:
        std::shared_ptr<asio::ip::tcp::socket> socket;
        std::shared_ptr<std::vector<std::int8_t>> buffer;
        ClientState state = ClientState::HANDSHAKE;
        Cerios::Server::Login *parent;
    public:
        Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, Cerios::Server::Login *parentPtr);
        void sendData(std::vector<std::int8_t> &data);
        void onLengthReceive(std::shared_ptr<asio::streambuf>, const asio::error_code &error, std::size_t bytes_transferred);
        void onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred);
        void setState(ClientState state);
        ClientState getState();
        std::shared_ptr<asio::ip::tcp::socket> getSocket();
    private:
        void startAsyncRead();
    };
}}

#endif /* Client_hpp */
