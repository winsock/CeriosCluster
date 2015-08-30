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
#include <asio.hpp>
#include <unordered_map>

struct sockaddr_in;
typedef uint32_t in_addr_t;
namespace Cerios { namespace Server {
    class Client;
    class ClientServer;
    class Login {
    private:
        asio::io_service service;
        asio::ip::tcp::acceptor clientAcceptor;
        asio::ip::tcp::acceptor clientServerAcceptor;
        std::unordered_map<std::uint32_t, std::shared_ptr<Cerios::Server::Client>> pendingClients;\
        using nodeMap = std::unordered_map<std::uint32_t, std::shared_ptr<Cerios::Server::ClientServer>>;
        nodeMap connectedNodes;
    public:
        Login(unsigned short mcPort, unsigned short nodeCommsPort, bool ipv6);
        void init();
        void listen();
        
        void handleClient(std::shared_ptr<asio::ip::tcp::socket> newClient, const asio::error_code &error);
        void handleNode(std::shared_ptr<asio::ip::tcp::socket> newNode, const asio::error_code &error);
        
        /**
         * Initial login suceeded, connect to actual game logic servers.
         */
        void handoffClient(std::shared_ptr<Cerios::Server::Client> client);

        bool checkAuth(std::string authtoken, int clientSocketHandle);
        void getFreeServerForClientWithToken(std::string authtoken, std::shared_ptr<Cerios::Server::Client> client);
        void clientDisconnected(std::shared_ptr<Cerios::Server::Client> disconnectedClient);
        in_addr_t getAddrFromHostname(std::string hostname, bool ipv6Prefered = false);
        ~Login();
    private:
        void tryGetClientServer();
        void asyncClientAccept();
        void asyncNodeAccept();
    };
}}

#endif /* LoginServer_hpp */
