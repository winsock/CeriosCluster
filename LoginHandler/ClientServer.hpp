//
//  ClientServer.hpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/30/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef ClientServer_hpp
#define ClientServer_hpp

#include <asio.hpp>
#include <unordered_map>
#include <memory>
#include "ClientOwner.hpp"

namespace Cerios { namespace Server {
    class Client;
    class Login;
    class ClientServer : public ClientOwner {
    private:
        std::shared_ptr<asio::ip::tcp::socket> socket;
        std::unordered_map<std::uint32_t, std::shared_ptr<Cerios::Server::Client>> clients;
        std::shared_ptr<Cerios::Server::Login> owner;
    public:
        ClientServer(std::shared_ptr<asio::ip::tcp::socket> serverEndpoint, std::shared_ptr<Cerios::Server::Login> owner);
        
        /**
         * Returns false if for some reason the client could not have been added to the client server
         **/
        bool addClient(std::shared_ptr<Cerios::Server::Client> client);
        
        /**
         * Tells the client server to gracefully shutdown
         **/
        void sendShutdownSignal();
        
        void onDataReceived(std::shared_ptr<asio::streambuf>, const asio::error_code &error, std::size_t bytes_transferred);
        void onWriteCompleteCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<std::function<void(void)>> callback, bool shutdownMessage = false);
        
        void clientDisconnected(std::shared_ptr<Cerios::Server::AbstractClient> disconnectedClient);
        bool onPacketReceived(std::shared_ptr<Cerios::Server::AbstractClient> client, std::shared_ptr<Cerios::Server::Packet> packet);
        
        std::weak_ptr<asio::io_service> getIOService();
    private:
        void startAsyncReceive();
    };
}}

#endif /* ClientServer_hpp */
