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
#include <AbstractClient.hpp>

namespace Cerios { namespace Server {
    class Login;
    class Client : public AbstractClient, public std::enable_shared_from_this<Client> {
    private:
        Cerios::Server::Login *parent;
    public:
        Client(std::shared_ptr<asio::ip::tcp::socket> clientSocket, Cerios::Server::Login *parentPtr);
        void sendData(std::vector<std::int8_t> &data);
        void sendData(std::vector<std::int8_t> &data, std::function<void(Cerios::Server::AbstractClient *)> &callback);
        void onLengthReceive(std::shared_ptr<asio::streambuf>, const asio::error_code &error, std::size_t bytes_transferred);
        void onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred);
        void onWriteCompleteCallback(const asio::error_code& error, std::size_t bytes_transferred, std::function<void(Cerios::Server::AbstractClient *)> &callback);
        void receivedMessage(std::shared_ptr<Cerios::Server::Packet> packet);
        void disconnect();
    private:
        void startAsyncRead();
    };
}}

#endif /* Client_hpp */
