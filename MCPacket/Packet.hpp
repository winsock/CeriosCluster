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
#include <sstream>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <iostream>
#include "AbstractClient.hpp"

struct PairHash {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const {
        return std::hash<std::string>()(std::string(std::to_string(x.first) + std::to_string(x.second)));
    }
};

#pragma GCC visibility push(default)
namespace Cerios { namespace Server {
    class Packet : public std::enable_shared_from_this<Packet> {
    public:
        using parsePacketFunction = std::shared_ptr<Packet>(std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        using newPacketFunction = std::shared_ptr<Packet>();
        using packetRegistryKeyType = std::pair<ClientState, std::int32_t>;
        using packet_registry = std::unordered_map<packetRegistryKeyType, std::pair<newPacketFunction *, parsePacketFunction *>, PairHash>;
        using packet_data_store = std::vector<std::int8_t>;
        
        static const std::uint32_t MAX_LENGTH_BYTES = 3; // Magic constant for max size in Minecraft
    protected:
        std::int32_t packetId;
        packet_data_store rawPayload;
    public:
        virtual ~Packet() = default;
        
        virtual void sendTo(Cerios::Server::AbstractClient *client) {}
        virtual void serializePacket(Cerios::Server::Side sideSending) {}
        
        static std::shared_ptr<Packet> parsePacket(std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer, ClientState state, bool consumeData = true);
        static std::shared_ptr<Packet> newPacket(ClientState state, std::int32_t packetId);
        
        static const std::size_t readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::int8_t> *buffer, std::size_t offset, bool consume = false);
        static const std::size_t readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::int8_t> *buffer, bool consume = false);
        
        static void registrate(ClientState const &state, std::int32_t const &packetId, newPacketFunction *np, parsePacketFunction *pp) {
            registry()[packetRegistryKeyType(state, packetId)] = std::pair<newPacketFunction *, parsePacketFunction *>(np, pp);
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
        void writeVarIntToBuffer(std::uint32_t input);
        void writeVarLongToBuffer(std::uint64_t input);
        void writeBufferLengthToFront();
        void write64bitInt(std::int64_t input);
        
    private:
        static std::shared_ptr<Packet> instantiateFromData(ClientState const &state, std::shared_ptr<Cerios::Server::Packet> packetInProgress) {
            auto it = registry().find(packetRegistryKeyType(state, packetInProgress->packetId));
            return it == registry().end() ? nullptr : (it->second.second)(packetInProgress);
        }
        
        static std::shared_ptr<Packet> instantiateNew(ClientState const &state, std::int32_t packetId) {
            auto it = registry().find(packetRegistryKeyType(state, packetId));
            return it == registry().end() ? nullptr : (it->second.first)();
        }
        static packet_registry &registry();
        Packet(std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer, ClientState state, bool consumeData = true);
    };
    
    class CompressedPacket {
        Packet uncompressedPacket;
    public:
        CompressedPacket(std::size_t length, std::shared_ptr<std::vector<std::int8_t>> buffer, bool consumeData = true);
    };
}}
#pragma GCC visibility pop

#endif /* Packet_hpp */
