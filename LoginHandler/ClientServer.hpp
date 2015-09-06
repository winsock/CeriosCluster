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
#include "ClientOwner.hpp"

namespace Cerios { namespace Server {
    class Client;
    class Login;
    
    typedef struct {
        std::uint8_t id;
        std::uint8_t packetNumber;
        std::uint32_t payloadLength;
    } MessagePacketHeader;
    
    class ClientServer : public ClientOwner {
    private:
        std::shared_ptr<asio::ip::udp::socket> sendSocket, receiveSocket;
        std::unordered_map<std::uint32_t, std::shared_ptr<Cerios::Server::Client>> clients;
        std::shared_ptr<Cerios::Server::Login> owner;
        std::vector<std::uint8_t> messageBuffer;

        std::vector<std::shared_ptr<asio::ip::udp::endpoint>> canAcceptClient;
    public:
        ClientServer(std::uint16_t messageReceive, bool ipv6, std::shared_ptr<Cerios::Server::Login> owner);
        
        /**
         * Returns false if for some reason the client could not have been added to the client server
         **/
        bool addClient(std::shared_ptr<Cerios::Server::Client> client);
        
        /**
         * Tells the client server to gracefully shutdown
         **/
        void sendShutdownSignal();
        
        void onDataReceived(std::shared_ptr<asio::ip::udp::endpoint> messageEndpoint, const asio::error_code &error, std::size_t bytes_transferred);
        void onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred);
        void onWriteCompleteCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<std::function<void(void)>> callback);
        
        void clientDisconnected(std::shared_ptr<Cerios::Server::AbstractClient> disconnectedClient);
        bool onPacketReceived(std::shared_ptr<Cerios::Server::AbstractClient> client, std::shared_ptr<Cerios::Server::Packet> packet);
        
        std::weak_ptr<asio::io_service> getIOService();
        std::shared_ptr<EVP_PKEY> getKeyPair();
        std::shared_ptr<X509> getCertificate();
    private:
        void startAsyncReceive();
    };
}}

#endif /* ClientServer_hpp */
