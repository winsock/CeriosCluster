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
#include <zlib.h>

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
        using parsePacketFunction = std::shared_ptr<Packet>(Cerios::Server::Side side, std::shared_ptr<Cerios::Server::Packet> packetInProgress);
        using newPacketFunction = std::shared_ptr<Packet>(Cerios::Server::Side side);
        using packetRegistryKeyType = std::pair<ClientState, std::int32_t>;
        using packet_registry = std::unordered_map<packetRegistryKeyType, std::pair<newPacketFunction *, parsePacketFunction *>, PairHash>;
        using packet_data_store = std::vector<std::uint8_t>;
        
        static const std::uint32_t MAX_LENGTH_BYTES = 3; // Magic constant for max size in Minecraft
    protected:
        std::int32_t packetId;
        packet_data_store rawPayload;
    public:
        virtual ~Packet() = default;
        
        void sendTo(Cerios::Server::AbstractClient *client, std::int32_t compressionThreshold);
        
        virtual void serializePacket(Cerios::Server::Side sideSending) {
            this->rawPayload.clear();
            this->writeVarIntToBuffer(this->packetId);
        }
        
        template <typename T = Packet>
        static std::shared_ptr<T> parsePacket(Cerios::Server::Side side, std::size_t length, std::shared_ptr<std::vector<std::uint8_t>> buffer, ClientState state, bool compressed = false, bool consumeData = true) {
            if (buffer->size() <= 0) {
                return nullptr;
            }
            
            if (compressed) {
                // Lambada to consume data. I made it a lambada so I wouldn't have to copy and paste this code in each error condition and after inflating.
                auto consumeFunc = [=]() -> void {
                    if (consumeData) {
                        buffer->erase(buffer->begin(), buffer->begin() + length);
                    }
                };
                
                std::int32_t inflatedPacketLength;
                readVarIntFromBuffer(&inflatedPacketLength, buffer.get());
                if (inflatedPacketLength > 0) {
                    // Packet buffer
                    std::shared_ptr<std::vector<std::uint8_t>> inflatedPacketBuffer(new std::vector<std::uint8_t>(inflatedPacketLength));
                    
                    // Compressed data.
                    z_stream zStream;
                    
                    zStream.zalloc = Z_NULL;
                    zStream.zfree = Z_NULL;
                    zStream.opaque = Z_NULL;
                    zStream.avail_in = 0;
                    zStream.next_in = Z_NULL;
                    
                    if (inflateInit(&zStream) != Z_OK) {
                        consumeFunc();
                        return nullptr;
                    }
                    
                    zStream.avail_in = static_cast<std::uint32_t>(buffer->size());
                    zStream.next_in = reinterpret_cast<std::uint8_t *>(buffer->data());
                    std::int32_t returnCode = 0;
                    std::size_t inflatedData = 0;
                    zStream.avail_out = inflatedPacketLength;
                    
                    do {
                        zStream.next_out = reinterpret_cast<std::uint8_t *>(inflatedPacketBuffer->data() + inflatedData);
                        returnCode = inflate(&zStream, Z_NO_FLUSH);
                        if (returnCode == Z_STREAM_ERROR) {
                            inflateEnd(&zStream);
                            consumeFunc();
                            return nullptr;
                        }
                    } while (inflatedData < inflatedPacketLength);
                    
                    inflateEnd(&zStream);
                    consumeFunc();
                    
                    if (inflatedPacketBuffer->size() != inflatedPacketLength) {
                        return nullptr;
                    }
                    return Packet::instantiateFromData<T>(side, state, std::shared_ptr<Cerios::Server::Packet>(new Cerios::Server::Packet(inflatedPacketLength, inflatedPacketBuffer, state, consumeData)));
                }
                // else data was smaller than the minimum compression limit set. Just a normal uncompressed packet. Drop down to normal packet handling.
            }
            return Packet::instantiateFromData<T>(side, state, std::shared_ptr<Cerios::Server::Packet>(new Cerios::Server::Packet(length, buffer, state, consumeData)));
        }
        
        template <typename T = Packet>
        static std::shared_ptr<T> newPacket(Cerios::Server::Side side, ClientState state, std::int32_t packetId) {
            return Packet::instantiateNew<T>(side, state, packetId);
        }
        
        static void registrate(ClientState const &state, std::int32_t const &packetId, newPacketFunction *np, parsePacketFunction *pp) {
            registry()[packetRegistryKeyType(state, packetId)] = std::pair<newPacketFunction *, parsePacketFunction *>(np, pp);
        }
        
        template <typename D>
        struct Registrar {
            explicit Registrar(ClientState const &state, std::int32_t const &packetId) {
                Packet::registrate(state, packetId, &D::newPacket, &D::parsePacket);
            }
        };
        
        static const std::size_t readVarIntFromBuffer(std::int32_t *intOut, std::vector<std::uint8_t> *buffer, bool consume = false);

    protected:
        Packet(std::shared_ptr<Cerios::Server::Packet> packetToCopy) : packetId(packetToCopy->packetId), rawPayload(packetToCopy->rawPayload) { }
        Packet(std::int32_t packetId) : packetId(packetId) {}
        template <typename T>
        T readPODFromBuffer(typename std::remove_reference<T>::type defaultReturn, bool consumeData = true) {
            if (this->rawPayload.size() < sizeof(T)) {
                return defaultReturn;
            }
            T pod;
            std::copy(this->rawPayload.data(), this->rawPayload.data() + sizeof(T), reinterpret_cast<std::uint8_t *>(&pod));
            if (consumeData) {
                this->rawPayload.erase(this->rawPayload.begin(), this->rawPayload.begin() + sizeof(T));
            }
            return pod;
        }
        
        void writeVarIntToBuffer(std::uint32_t input);
        void writeVarLongToBuffer(std::uint64_t input);
        void writeVarIntToFront(std::vector<std::uint8_t> *buffer, std::int32_t);
        void writeBufferLengthToFront(std::vector<std::uint8_t> *buffer);
        void writeBufferLengthToFront();
        
        template <typename T>
        void writePODToBuffer(T &podData) {
            std::copy_n(reinterpret_cast<std::uint8_t *>(&podData), sizeof(T), std::back_inserter(this->rawPayload));
        }
        
    private:
        template <typename T = Packet>
        static std::shared_ptr<T> instantiateFromData(Cerios::Server::Side side, ClientState const &state, std::shared_ptr<Cerios::Server::Packet> packetInProgress) {
            auto it = registry().find(packetRegistryKeyType(state, packetInProgress->packetId));
            return std::dynamic_pointer_cast<T>(it == registry().end() ? nullptr : (it->second.second)(side, packetInProgress));
        }
        
        template <typename T = Packet>
        static std::shared_ptr<T> instantiateNew(Cerios::Server::Side side, ClientState const &state, std::int32_t packetId) {
            auto it = registry().find(packetRegistryKeyType(state, packetId));
            return std::dynamic_pointer_cast<T>(it == registry().end() ? nullptr : (it->second.first)(side));
        }
        static packet_registry &registry();
        Packet(std::size_t length, std::shared_ptr<std::vector<std::uint8_t>> buffer, ClientState state, bool consumeData = true);
    };
}}
#pragma GCC visibility pop

#endif /* Packet_hpp */
