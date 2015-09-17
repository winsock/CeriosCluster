//
//  ClientServer.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/30/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "ClientServer.hpp"

#include <iostream>
#include <Packet.hpp>
#include <JoinGamePacket.hpp>
#include <SpawnPositionPacket.hpp>
#include <PlayerPositionAndLookPacket.hpp>
#include <PlayerAbilitiesPacket.hpp>

#include "Client.hpp"
#include "LoginServer.hpp"

Cerios::Server::ClientServer::ClientServer(std::uint16_t messageReceive, bool ipv6, std::shared_ptr<Cerios::Server::Login> owner) :
sendSocket(new asio::ip::udp::socket(*owner->getIOService().lock())),
receiveSocket(new asio::ip::udp::socket(*owner->getIOService().lock())), owner(owner) {
    // Enable broadcast
    sendSocket->open(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4());
    sendSocket->set_option(asio::socket_base::reuse_address(true));
    sendSocket->set_option(asio::socket_base::broadcast(true));
    sendSocket->bind(asio::ip::udp::endpoint(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4(), 0));
    
    receiveSocket->open(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4());
    receiveSocket->set_option(asio::socket_base::reuse_address(true));
    receiveSocket->bind(asio::ip::udp::endpoint(ipv6 ? asio::ip::udp::v6() : asio::ip::udp::v4(), messageReceive));
    
    this->startAsyncReceive();
}

void Cerios::Server::ClientServer::startAsyncReceive() {
    this->receiveSocket->async_wait(asio::socket_base::wait_read, std::bind(&Cerios::Server::ClientServer::onDataReceived, this, std::placeholders::_1));
}

void Cerios::Server::ClientServer::onDataReceived(const asio::error_code &error) {
    if (error) {
        std::cerr<<error.message()<<std::endl;
        return;
    }
    
    std::size_t bytesAvailable = this->receiveSocket->available();
    std::vector<std::uint8_t> data(bytesAvailable);
    
    asio::ip::udp::endpoint senderEndpoint;
    asio::error_code receiveError;
    std::size_t packetSize = this->receiveSocket->receive_from(asio::buffer(data, bytesAvailable), senderEndpoint, 0, receiveError);
    if (receiveError) {
        std::cerr<<receiveError.message()<<std::endl;
        return;
    }
    
    data.resize(packetSize);
    this->handleMessage(senderEndpoint, Cerios::InternalComms::Packet::fromData(data));
    
    this->startAsyncReceive();
}

void Cerios::Server::ClientServer::handleMessage(asio::ip::udp::endpoint &endpoint, std::shared_ptr<Cerios::InternalComms::Packet> packet) {
    if (this->clients[packet->getPlayerID()] == nullptr) {
        return;
    }
    
    switch (packet->getMessageID()) {
        case Cerios::InternalComms::MessageID::MC_PACKET: {
            if (this->clients[packet->getPlayerID()] != nullptr) {
                std::shared_ptr<std::vector<std::uint8_t>> payload = packet->getPayload().lock();
                if (payload->size() >= this->clients[packet->getPlayerID()]->getCompressionThreshold()) {
                    Cerios::Server::Packet::compressData(*payload);
                }
                Cerios::Server::Packet::writeBufferLengthToFront(*payload);
                this->clients[packet->getPlayerID()]->sendData(*payload);
            }
        } break;
        default:
            break;
    }
}

void Cerios::Server::ClientServer::onWriteComplete(const asio::error_code& error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr<<"Internal write error from clientserver node: "<<error.message()<<std::endl;
    }
}

void Cerios::Server::ClientServer::sendPacket(std::shared_ptr<Cerios::InternalComms::Packet> message) {
    std::vector<std::uint8_t> rawData;
    message->serializeData(rawData);
    this->sendSocket->async_send_to(asio::buffer(rawData), asio::ip::udp::endpoint(asio::ip::address_v4::broadcast(), 1337), std::bind(&Cerios::Server::ClientServer::onWriteComplete, this, std::placeholders::_1, std::placeholders::_2));
}

void Cerios::Server::ClientServer::onWriteCompleteCallback(const asio::error_code& error, std::size_t bytes_transferred, std::shared_ptr<std::function<void(void)>> callback) {
    onWriteComplete(error, bytes_transferred);
    (*callback)();
}

void Cerios::Server::ClientServer::sendShutdownSignal() {
    for (auto &clientMapPair : this->clients) {
        clientMapPair.second->disconnect();
    }
    if (this->receiveSocket->is_open()) {
        std::shared_ptr<std::function<void(std::shared_ptr<asio::ip::udp::socket> socket)>> functionLambada(new std::function<void(std::shared_ptr<asio::ip::udp::socket> socket)>([](std::shared_ptr<asio::ip::udp::socket> socket) -> void{
            if (socket->is_open()) {
                socket->close();
            }
        }));
        (*functionLambada)(this->sendSocket);
        (*functionLambada)(this->receiveSocket);
//        asio::async_write(*this->socket, asio::buffer(std::string("bye")), std::bind(&Cerios::Server::ClientServer::onWriteCompleteCallback, this, std::placeholders::_1, std::placeholders::_2, functionLambada));
    }
}

bool Cerios::Server::ClientServer::addClient(std::unique_ptr<Cerios::Server::Client> client) {
//    std::vector<std::uint8_t> payload;
//    std::copy_n(client->getClientId().begin(), client->getClientId().length(), std::back_inserter(payload));
//    std::shared_ptr<Cerios::InternalComms::Packet> message = Cerios::InternalComms::Packet::newPacket(Cerios::InternalComms::MessageID::ACCEPT_CLIENT, client->getClientId(), payload);

//    this->sendPacket(message);
    std::cout<<client->getClientUsername()<<"("<<client->getClientId()<<") connected!"<<std::endl;
    
    auto mysqldb = this->owner->getMySQLConnection();
    mysqlpp::Query query = mysqldb->query("SELECT * FROM `players` WHERE `uuid`='" + client->getClientId() + "';");
    mysqlpp::StoreQueryResult res = query.store();
    if (res && res.size() > 0) {
        // Previous player
        // TODO get last location/game state info and world id, etc and send it to the client to sync.
    } else {
        // New user add to db
        mysqldb->query().exec("INSERT INTO `players` (uuid) VALUES ('" + client->getClientId() + "');");
        // TODO get overworld spawn and set it in the db for the player
    }
    
    auto joinGamePacket = Packet::newPacket<Cerios::Server::JoinGamePacket>(Cerios::Server::Side::SERVER, Cerios::Server::ClientState::PLAY, 0x01);
    client->sendPacket(joinGamePacket);
    auto spawnPosition = Packet::newPacket<Cerios::Server::SpawnPositionPacket>(Cerios::Server::Side::SERVER, Cerios::Server::ClientState::PLAY, 0x05);
    client->sendPacket(spawnPosition);
    auto posAndLook = Packet::newPacket<Cerios::Server::PlayerPositionAndLookPacket>(Cerios::Server::Side::SERVER, Cerios::Server::ClientState::PLAY, 0x08);
    client->sendPacket(posAndLook);
    auto setAbilities = Packet::newPacket<Cerios::Server::PlayerAbilitiesPacket>(Cerios::Server::Side::SERVER, Cerios::Server::ClientState::PLAY, 0x39);
    client->sendPacket(setAbilities);
    
    this->clients[client->getClientId()] = std::move(client);
    return true;
}

std::weak_ptr<asio::io_service> Cerios::Server::ClientServer::getIOService() {
    return this->owner->getIOService();
}

/** 
 * TODO Research TCP Handoff.
 * We're redirecting the packet to be handled by the client server. Cancel the login/status packet parsing
 * Most likely only viable on Linux. Might be possible on OS X with porting a BSD driver into a kext.
**/
bool Cerios::Server::ClientServer::onPacketReceived(Cerios::Server::Client *client, std::shared_ptr<Cerios::Server::Packet> packet) {
    std::vector<std::uint8_t> payload;
    packet->serializeToBuffer(Cerios::Server::Side::CLIENT, payload);
    std::shared_ptr<Cerios::InternalComms::Packet> message = Cerios::InternalComms::Packet::newPacket(Cerios::InternalComms::MessageID::MC_PACKET, client->getClientId(), payload);
    this->sendPacket(message);
    
    return true;
}

void Cerios::Server::ClientServer::clientDisconnected(Cerios::Server::Client *disconnectedClient) {
    this->clients.erase(disconnectedClient->getClientId());
    try {
        std::cout<<"Client "<<disconnectedClient->getSocket()->remote_endpoint()<<" Disconnected!"<<std::endl;
    } catch (...) {}
}

std::shared_ptr<EVP_PKEY> Cerios::Server::ClientServer::getKeyPair() {
    return owner->getKeyPair();
}

std::shared_ptr<X509> Cerios::Server::ClientServer::getCertificate() {
    return owner->getCertificate();
}

std::string Cerios::Server::ClientServer::getPublicKeyString() {
    return owner->getPublicKeyString();
}