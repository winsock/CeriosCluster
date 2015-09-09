//
//  LoginServer.hpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef LoginServer_hpp
#define LoginServer_hpp

#include <string>
#include <map>
#include <vector>
#include <thread>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "ClientOwner.hpp"

struct sockaddr_in;
typedef uint32_t in_addr_t;
namespace Cerios { namespace Server {
    class Client;
    class ClientServer;
    typedef std::chrono::duration<std::int32_t, std::ratio<86400>> Days;
    class Login : public ClientOwner {
    private:
        std::shared_ptr<asio::io_service> service;
        asio::ip::tcp::acceptor clientAcceptor;
        asio::ip::tcp::acceptor clientServerAcceptor;
        std::shared_ptr<ClientServer> clientServerHanler;
        std::unordered_map<std::uint32_t, std::unique_ptr<Cerios::Server::Client>> pendingClients;
        std::shared_ptr<EVP_PKEY> keyPair;
        std::shared_ptr<X509> certificate;
    public:
        Login(unsigned short mcPort, unsigned short nodeCommsPort, bool ipv6);
        void init();
        void listen();
        
        void handleClient(std::shared_ptr<asio::ip::tcp::socket> newClient, const asio::error_code &error);
        
        /**
         * Accessors for the login node's encryption stuff.
         **/
        std::shared_ptr<EVP_PKEY> getKeyPair();
        std::shared_ptr<X509> getCertificate();
        
        /**
         * Initial login suceeded, connect to actual game logic servers.
         */
        void handoffClient(Cerios::Server::AbstractClient *client);

        void clientDisconnected(Cerios::Server::AbstractClient *disconnectedClient);
        std::weak_ptr<asio::io_service> getIOService();
        ~Login();
    private:
        void tryGetClientServer();
        void asyncClientAccept();
    };
}}

#endif /* LoginServer_hpp */
