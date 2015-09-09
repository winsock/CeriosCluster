//
//  AbstractClient.hpp
//  MCPacket
//
//  Created by Andrew Querol on 8/30/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef AbstractClient_hpp
#define AbstractClient_hpp

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <asio.hpp>

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class Packet;
    enum ClientState {
        HANDSHAKE = 0,
        STATUS = 1,
        LOGIN = 2,
        PLAY = 3
    };
    enum Side {
        CLIENT = 0,
        SERVER = 1,
        BOTH = 2
    };
    class AbstractClient {
    protected:
        std::shared_ptr<asio::ip::tcp::socket> socket;
        std::shared_ptr<std::vector<std::uint8_t>> buffer;
        ClientState state = ClientState::HANDSHAKE;
    public:
        AbstractClient(std::shared_ptr<asio::ip::tcp::socket> clientConnection) : socket(clientConnection), buffer(new std::vector<std::uint8_t>()) {}
        virtual ClientState getState() { return state; }
        virtual Side getSide() = 0;
        virtual void setState(ClientState state) { this->state = state; }
        virtual std::shared_ptr<asio::ip::tcp::socket> getSocket() { return socket; }
        virtual void disconnect() = 0;
        virtual void sendData(std::vector<std::uint8_t> &data) = 0;
        virtual void receivedMessage(Side side, std::shared_ptr<Cerios::Server::Packet> packet) = 0;
    };
}}
#pragma GCC visibility pop

#endif /* AbstractClient_hpp */
