//
//  ClientServer.hpp
//  Client Server
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef ClientServer_hpp
#define ClientServer_hpp
#include <vector>
#include <cstdint>
#include <InternalComms.hpp>

#include <asio.hpp>

namespace Cerios { namespace Server {
    class GameState;
    class ClientServer {
    private:
        std::shared_ptr<asio::io_service> service;
        asio::ip::udp::socket sendSocket, receiveSocket;
        
        std::vector<GameState> players;
    public:
        ClientServer(std::uint16_t receivePort, bool ipv6);
        void init();
        void listen();
        void onDatagramMessageReceived(const asio::error_code& error);
        void handleMessage(asio::ip::udp::endpoint &endpoint, Cerios::InternalComms::Packet &packet);
        ~ClientServer();
    private:
        void startReceive();
    };
}}

#endif /* ClientServer_hpp */
