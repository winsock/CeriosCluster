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

#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/err.h>

#if defined(OPENSSL_IS_BORINGSSL)
extern "C" {
#if !defined(SSL_R_SHORT_READ)
# define SSL_R_SHORT_READ    SSL_R_UNEXPECTED_RECORD
#endif // !defined(SSL_R_SHORT_READ)
    inline void CONF_modules_unload(int p) {}
#undef ERR_PACK
#define ERR_PACK(lib, int, reason) \
(((((uint32_t)lib) & 0xff) << 24) | ((((uint32_t)reason) & 0xfff)))
}
#endif // defined(OPENSSL_IS_BORINGSSL)
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <asio/read.hpp>

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
        std::unordered_map<std::uint32_t, std::shared_ptr<Cerios::Server::Client>> pendingClients;\
        using nodeMap = std::unordered_map<std::uint32_t, std::shared_ptr<Cerios::Server::ClientServer>>;
        nodeMap connectedNodes;
        std::shared_ptr<EVP_PKEY> keyPair;
        std::shared_ptr<X509> certificate;
    public:
        Login(unsigned short mcPort, unsigned short nodeCommsPort, bool ipv6);
        void init();
        void listen();
        
        void handleClient(std::shared_ptr<asio::ip::tcp::socket> newClient, const asio::error_code &error);
        void handleNode(std::shared_ptr<asio::ip::tcp::socket> newNode, const asio::error_code &error);
        
        /**
         * Accessors for the login node's encryption stuff.
         **/
        std::shared_ptr<EVP_PKEY> getKeyPair();
        std::shared_ptr<X509> getCertificate();
        
        /**
         * Initial login suceeded, connect to actual game logic servers.
         */
        void handoffClient(std::shared_ptr<Cerios::Server::Client> client);

        bool checkAuth(std::string authtoken, int clientSocketHandle);
        void getFreeServerForClientWithToken(std::string authtoken, std::shared_ptr<Cerios::Server::Client> client);
        void clientDisconnected(std::shared_ptr<Cerios::Server::AbstractClient> disconnectedClient);
        in_addr_t getAddrFromHostname(std::string hostname, bool ipv6Prefered = false);
        std::weak_ptr<asio::io_service> getIOService();
        ~Login();
    private:
        void tryGetClientServer();
        void asyncClientAccept();
        void asyncNodeAccept();
    };
}}

#endif /* LoginServer_hpp */
