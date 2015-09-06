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
#include <vector>
#include <cstdint>

#include <asio.hpp>

namespace Cerios { namespace Server {
    class GameState;
    class ClientServer {
        typedef struct {
            std::uint8_t id;
            std::uint8_t packetNumber;
            std::uint32_t payloadLength;
        } MessagePacketHeader;
        
    private:
        std::shared_ptr<asio::io_service> service;
        asio::ip::udp::socket socket;
        std::vector<std::uint8_t> messageBuffer;
        
        std::vector<GameState> players;
    public:
        ClientServer(unsigned short nodeCommsPort, bool ipv6);
        void init();
        void listen();
        void onDatagramMessageReceived(std::shared_ptr<asio::ip::udp::endpoint> messageEndpoint, const asio::error_code& error, std::size_t bytes_transferred);
        ~ClientServer();
    private:
        void startReceive();
    };
}}

#endif /* ClientServer_hpp */
