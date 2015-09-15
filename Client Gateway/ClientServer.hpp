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
#include <vector>

#include <InternalComms.hpp>

#include "ClientOwner.hpp"

namespace Cerios { namespace Server {
    class Client;
    class Login;
    class ClientServer : public ClientOwner {
    private:
        std::shared_ptr<asio::ip::udp::socket> sendSocket, receiveSocket;
        std::unordered_map<std::string, std::unique_ptr<Cerios::Server::Client>> clients;
        std::shared_ptr<Cerios::Server::Login> owner;
    public:
        ClientServer(std::uint16_t messageReceive, bool ipv6, std::shared_ptr<Cerios::Server::Login> owner);
        
        /**
         * Returns false if for some reason the client could not have been added to the client server
         **/
        bool addClient(std::unique_ptr<Cerios::Server::Client> client);
        
        /**
         * Tells the client server to gracefully shutdown
         **/
        void sendShutdownSignal();
        
        void onDataReceived(const asio::error_code &error);
        void onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred);
        void onWriteCompleteCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<std::function<void(void)>> callback);
        void handleMessage(asio::ip::udp::endpoint &endpoint, std::shared_ptr<Cerios::InternalComms::Packet> packet);
        void sendPacket(std::shared_ptr<Cerios::InternalComms::Packet> message);

        void clientDisconnected(Cerios::Server::Client *disconnectedClient);
        bool onPacketReceived(Cerios::Server::Client *client, std::shared_ptr<Cerios::Server::Packet> packet);
        
        std::weak_ptr<asio::io_service> getIOService();
        
        std::shared_ptr<EVP_PKEY> getKeyPair();
        std::shared_ptr<X509> getCertificate();
        std::string getPublicKeyString();

    private:
        void startAsyncReceive();
    };
}}

#endif /* ClientServer_hpp */
