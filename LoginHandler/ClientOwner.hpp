//
//  ClientOwner.hpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/31/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef ClientOwner_hpp
#define ClientOwner_hpp

#include <memory>

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

#include <Packet.hpp>

namespace Cerios { namespace Server {
    class Client;
    class ClientOwner : public std::enable_shared_from_this<ClientOwner> {
    public:
        virtual std::weak_ptr<asio::io_service> getIOService() = 0;
        
        virtual void clientDisconnected(Cerios::Server::Client *disconnectedClient) = 0;
        virtual bool onPacketReceived(Side side, Cerios::Server::Client *client, std::shared_ptr<Packet> packet) { return true; }
        virtual void handoffClient(Cerios::Server::Client *client) {}
        
        /**
         * Accessors for the login node's encryption stuff.
         **/
        virtual std::shared_ptr<EVP_PKEY> getKeyPair() = 0;
        virtual std::shared_ptr<X509> getCertificate() = 0;
        virtual std::string getPublicKeyString() = 0;
    };
}}

#endif /* ClientOwner_hpp */
