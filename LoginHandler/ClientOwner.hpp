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
#include <AbstractClient.hpp>

namespace Cerios { namespace Server {
    class ClientOwner : public std::enable_shared_from_this<ClientOwner> {
    public:
        virtual void clientDisconnected(std::shared_ptr<Cerios::Server::AbstractClient> disconnectedClient) = 0;
        virtual bool onPacketReceived(Side side, std::shared_ptr<AbstractClient> client, std::shared_ptr<Packet> packet) { return true; }
    };
}}

#endif /* ClientOwner_hpp */
