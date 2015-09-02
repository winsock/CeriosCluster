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
#include <openssl/x509v3.h>
#include <asio.hpp>

#include <AbstractClient.hpp>

namespace Cerios { namespace Server {
    class ClientOwner : public std::enable_shared_from_this<ClientOwner> {
    public:
        virtual std::weak_ptr<asio::io_service> getIOService() = 0;
        
        virtual void clientDisconnected(std::shared_ptr<Cerios::Server::AbstractClient> disconnectedClient) = 0;
        virtual bool onPacketReceived(Side side, std::shared_ptr<AbstractClient> client, std::shared_ptr<Packet> packet) { return true; }
        
        /**
         * Accessors for the login node's encryption stuff.
         **/
        virtual std::shared_ptr<EVP_PKEY> getKeyPair() { return nullptr; }
        virtual std::shared_ptr<X509> getCertificate() { return nullptr; }
    };
}}

#endif /* ClientOwner_hpp */
