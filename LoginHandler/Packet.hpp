//
//  Packet.hpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/29/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef Packet_hpp
#define Packet_hpp

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

struct PairHash {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const {
        return std::hash<std::string>()(std::string(std::to_string(x.first) + std::to_string(x.second)));
    }
};

namespace Cerios { namespace Server {
    class Client;
    
    typedef enum : std::int32_t {
        HANDSHAKE = 0,
        STATUS = 1,
        LOGIN = 2,
        PLAY = 3
    } ClientState;
    
    class Packet {
    public:
        using parsePacketFunction = std::shared_ptr<Packet>(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        using newPacketFunction = std::shared_ptr<Packet>();
        using packet_registry = std::unordered_map<std::pair<ClientState, std::int32_t>, std::pair<newPacketFunction *, parsePacketFunction *>, PairHash>;
        using packet_data_store = std::vector<std::int8_t>;
    protected:
        std::int32_t packetId;
        packet_data_store rawPayload;
    public:
        virtual ~Packet() = default;
        
        virtual void onReceivedBy(Cerios::Server::Client *client) {}
        virtual void sendTo(Cerios::Server::Client *client) {}
        virtual void serializePacket() {}
        
        static std::shared_ptr<Packet> parsePacket(std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer, ClientState state, bool consumeData = true);
        static std::shared_ptr<Packet> newPacket(ClientState state, std::int32_t packetId);
        
        static const std::size_t readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::int8_t> *buffer, std::size_t offset, bool consume = false);
        static const std::size_t readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::int8_t> *buffer, bool consume = false);
        
        static void registrate(ClientState const &state, std::int32_t const &packetId, newPacketFunction *np, parsePacketFunction *pp) {
            registry()[std::pair<ClientState, std::int32_t>(state, packetId)] = std::pair<newPacketFunction *, parsePacketFunction *>(np, pp);
        }
        
        template <typename D>
        struct Registrar {
            explicit Registrar(ClientState const &state, std::int32_t const &packetId) {
                Packet::registrate(state, packetId, &D::newPacket, &D::parsePacket);
            }
        };
    protected:
        Packet(std::shared_ptr<Cerios::Server::Packet> packetToCopy) : packetId(packetToCopy->packetId), rawPayload(packetToCopy->rawPayload) { }
        Packet(std::int32_t packetId) : packetId(packetId) {}
        void writeVarIntToBuffer(std::int32_t input);
        void writeVarLongToBuffer(std::int64_t input);
        void writeBufferLengthToFront();
        
        static std::shared_ptr<Packet> instantiateFromData(ClientState const &state, std::shared_ptr<Cerios::Server::Packet> packetInProgress) {
            auto it = registry().find(std::pair<ClientState, std::int32_t>(state, packetInProgress->packetId));
            return it == registry().end() ? nullptr : (it->second.second)(packetInProgress);
        }
        
        static std::shared_ptr<Packet> instantiateNew(ClientState const &state, std::int32_t packetId) {
            auto it = registry().find(std::pair<ClientState, std::int32_t>(state, packetId));
            return it == registry().end() ? nullptr : (it->second.first)();
        }
    private:
        static packet_registry &registry();
        Packet(std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer, ClientState state, bool consumeData = true);
    };
    
    class CompressedPacket {
        Packet uncompressedPacket;
    public:
        CompressedPacket(std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer, bool consumeData = true);
    };
}}

#endif /* Packet_hpp */
