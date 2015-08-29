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
    class Login {
    private:
        asio::io_service service;
        asio::ip::tcp::acceptor acceptor;
        std::unordered_map<std::uint32_t, std::shared_ptr<Cerios::Server::Client>> clients;
    public:
        Login(unsigned short port, bool ipv6) : acceptor(std::ref(service), asio::ip::tcp::endpoint(ipv6 ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), port)) {}
        void init(std::string multicastGroupAddr);
        void listen();
        void handle_receive(std::shared_ptr<asio::ip::tcp::socket> newClient, const asio::error_code &error);
        bool checkAuth(std::string authtoken, int clientSocketHandle);
        void getFreeServerForClientWithToken(std::string authtoken, int clientHandle);
        void clientDisconnected(Cerios::Server::Client *disconnectedClient);
        in_addr_t getAddrFromHostname(std::string hostname, bool ipv6Prefered = false);
        ~Login();
    private:
        void tryGetClientServer();
        void asyncAccept();
    };
}}

#endif /* LoginServer_hpp */
