//
//  LoginServer.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "LoginServer.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <cstdlib>
#include <chrono>


#include "Client.hpp"
#include "ClientServer.hpp"
#include "RandomSelect.hpp"

Cerios::Server::Login::Login(unsigned short mcPort, unsigned short nodeCommsPort, bool ipv6) :
clientAcceptor(std::ref(service), asio::ip::tcp::endpoint(ipv6 ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), mcPort)),
clientServerAcceptor(std::ref(service), asio::ip::tcp::endpoint(ipv6 ? asio::ip::tcp::v6() : asio::ip::tcp::v4(), nodeCommsPort)),
keyPair(EVP_PKEY_new(), [=](EVP_PKEY* keyPair) { EVP_PKEY_free(keyPair); }),
certificate(X509_new(), [=](X509* cert) { X509_free(cert); }) {
    
}

void Cerios::Server::Login::listen() {
        this->service.run();

}

void Cerios::Server::Login::asyncClientAccept() {
    std::shared_ptr<asio::ip::tcp::socket> tmpSocket = std::shared_ptr<asio::ip::tcp::socket>(new asio::ip::tcp::socket(clientAcceptor.get_executor().context()));
    clientAcceptor.async_accept(*tmpSocket, std::bind(&Cerios::Server::Login::handleClient, this, tmpSocket, std::placeholders::_1));
}

void Cerios::Server::Login::asyncNodeAccept() {
    std::shared_ptr<asio::ip::tcp::socket> tmpSocket = std::shared_ptr<asio::ip::tcp::socket>(new asio::ip::tcp::socket(clientServerAcceptor.get_executor().context()));
    clientServerAcceptor.async_accept(*tmpSocket, std::bind(&Cerios::Server::Login::handleNode, this, tmpSocket, std::placeholders::_1));
}

void Cerios::Server::Login::init() {
    RSA *rsa = RSA_new();
    BIGNUM *bne = BN_new();
    BN_set_word(bne, RSA_F4);
    RSA_generate_key_ex(rsa, 1024, bne, nullptr);
    BN_free(bne);
    EVP_PKEY_assign_RSA(this->keyPair.get(), rsa);
    

    ASN1_INTEGER_set(X509_get_serialNumber(this->certificate.get()), 1);
    X509_gmtime_adj(X509_get_notBefore(this->certificate.get()), 0);
    X509_gmtime_adj(X509_get_notAfter(this->certificate.get()), std::chrono::duration_cast<std::chrono::seconds>(Cerios::Server::Days(365)).count());
    X509_set_pubkey(this->certificate.get(), this->keyPair.get());
    X509_NAME *name;
    name = X509_get_subject_name(this->certificate.get());
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("US"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, reinterpret_cast<const unsigned char *>("Cerios Cluster Server"), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char *>("localhost"), -1, -1, 0);
    X509_set_issuer_name(this->certificate.get(), name);
    X509_sign(this->certificate.get(), this->keyPair.get(), EVP_sha256());

    this->asyncNodeAccept();
    this->asyncClientAccept();
}

void Cerios::Server::Login::handleClient(std::shared_ptr<asio::ip::tcp::socket> newClient, const asio::error_code &error) {
    if (!error) {
        std::shared_ptr<Cerios::Server::Client> client(new Cerios::Server::Client(newClient, this->shared_from_this()));
        pendingClients[newClient->native_handle()] = client;
    }
    this->asyncClientAccept();
}

void Cerios::Server::Login::handleNode(std::shared_ptr<asio::ip::tcp::socket> newNode, const asio::error_code &error) {
    if (!error) {
        std::cout<<newNode->remote_endpoint()<<std::endl;
        std::shared_ptr<Cerios::Server::ClientServer> node(new Cerios::Server::ClientServer(newNode));
        this->connectedNodes[newNode->native_handle()] = node;
    }
    this->asyncNodeAccept();
}

std::shared_ptr<EVP_PKEY> Cerios::Server::Login::getKeyPair() {
    return this->keyPair;
}

std::shared_ptr<X509> Cerios::Server::Login::getCertificate() {
    return this->certificate;
}

void Cerios::Server::Login::clientDisconnected(std::shared_ptr<Cerios::Server::AbstractClient> disconnectedClient) {
    std::cout<<"Client "<<disconnectedClient->getSocket()->remote_endpoint()<<" Disconnected!"<<std::endl;
    try {
        if (disconnectedClient->getSocket()->is_open()) {
            disconnectedClient->getSocket()->shutdown(asio::ip::tcp::socket::shutdown_both);
            disconnectedClient->getSocket()->close();
        }
    } catch (...) { }
    
    pendingClients.erase(disconnectedClient->getSocket()->native_handle());
}

void Cerios::Server::Login::handoffClient(std::shared_ptr<Cerios::Server::Client> client) {
    if (this->connectedNodes.size() <= 0) {
        return;
    }
    RandomSelect<> randomSelection{};
    uint8_t maxAttemps = 16;
    while (maxAttemps > 0) {
        auto random = randomSelection(this->connectedNodes).second;
        if (random->addClient(client)) {
            client->setOwner(random->shared_from_this());
            this->pendingClients.erase(client->getSocket()->native_handle());
            return;
        }
        maxAttemps--;
    }
}

bool Cerios::Server::Login::checkAuth(std::string authtoken, int clientSocketHandle) {
//    int socketDiscriptor;
//    struct sockaddr_in authServerAddress;
//    SSL_CTX *sslContext;
//    SSL *ssl;
//    
//    socketDiscriptor = socket(AF_INET, SOCK_STREAM, 0);
//    if (socketDiscriptor < 0) {
//        return false;
//    }
//    
//    SSLeay_add_ssl_algorithms();
//    SSL_load_error_strings();
//    sslContext = SSL_CTX_new(TLS_client_method());
//    if (sslContext == nullptr) {
//        return false;
//    }
//    memset(&authServerAddress, 0, sizeof(authServerAddress));
//    authServerAddress.sin_family      = AF_INET;
//    authServerAddress.sin_addr.s_addr = this->getAddrFromHostname("authserver.mojang.com");   /* Server Address */
//    authServerAddress.sin_port        = htons(443);          /* Server Port number */
//    
//    if (connect(socketDiscriptor, (struct sockaddr*) &authServerAddress, sizeof(authServerAddress)) < 0) {
//        return false;
//    }
//    
//    /* ----------------------------------------------- */
//    /* Now we have TCP conncetion. Start SSL negotiation. */
//    
//    ssl = SSL_new(sslContext);
//    if (ssl == nullptr) {
//        return false;
//    }
//    SSL_set_fd(ssl, socketDiscriptor);
//    if (SSL_connect(ssl) < 0) {
//        return false;
//    }
//    
//    /* Get the cipher - opt
//    
//    std::cout<<"SSL connection using: "<<SSL_get_cipher(ssl)<<std::endl; */
//    
//    /* Get server's certificate (note: beware of dynamic allocation) - opt
//    X509 *server_cert = SSL_get_peer_certificate(ssl);
//    
//    X509_free(server_cert); */
//    
//    /**
//     * Do auth check here
//     */
//    std::string content =
//    "{\r\n"
//    "\"accessToken\": \"" + authtoken + "\"\r\n"
//    "}";
//    
//    std::string requestString =
//    "POST /validate HTTP/1.1\r\n"
//    "Host: authserver.mojang.com\r\n"
//    "User-Agent: CeriosCluster\r\n"
//    "Accept: application/json\r\n"
//    "Content-Type: application/json; charset=UTF-8\r\n"
//    "Content-Length: " + std::to_string(content.length()) + "\r\n"
//    "Connection: Close\r\n"
//    "\r\n" + content;
//    SSL_write(ssl, requestString.c_str(), static_cast<int>(requestString.length()));
//    std::stringbuf responseBuffer;
//    
//    char buffer[4096];
//    size_t contentLength;
//    std::string contentBreakToken = "\r\n\r\n";
//    std::string contentLengthToken = "Content-Length: ";
//
//    while (true){
//        int lengthReceived = SSL_read(ssl, &buffer, sizeof(buffer));
//        if (lengthReceived < 0)
//            break;
//        if (lengthReceived == 0)
//            break;
//        responseBuffer.sputn(buffer, lengthReceived);
//        
//        size_t contentBreak = 0;
//        if ((contentBreak = responseBuffer.str().find(contentBreakToken)) > 0 && contentBreak != std::string::npos) {
//            if (contentBreak + contentBreakToken.length() >= responseBuffer.str().length()) {
//                break;
//            }
//            
//            size_t contentLengthLocation = responseBuffer.str().find(contentLengthToken);
//            if (contentLengthLocation == std::string::npos) {
//                break;
//            }
//            
//            size_t contentLengthEnd = responseBuffer.str().find("\r\n", contentLengthLocation + contentLengthToken.length());
//            if (contentLengthEnd == std::string::npos) {
//                break;
//            }
//            
//            std::string contentLengthString = responseBuffer.str().substr(contentLengthLocation + contentLengthToken.length(), contentLengthEnd);
//            contentLength = std::stoul(contentLengthString, nullptr, 0);
//            if (contentBreak + contentBreakToken.length() + contentLength >= responseBuffer.str().length()) {
//                break;
//            }
//        }
//    };
//    
//    SSL_shutdown(ssl);
//    
//    close(socketDiscriptor);
//    SSL_free(ssl);
//    SSL_CTX_free(sslContext);
//    
//    return responseBuffer.str().find("204 No Content") != std::string::npos;
    return false;
}

void Cerios::Server::Login::getFreeServerForClientWithToken(std::string authToken, std::shared_ptr<Cerios::Server::Client> client) {
//    /* Send a message to the multicast group specified by the*/
//    /* groupSock sockaddr structure. */
//    if(sendto(this->broadcastSocketHandle, authToken.data(), sizeof(authToken.data()[0]) * authToken.size(), 0, (struct sockaddr*)this->groupSocketDiscription, sizeof(sockaddr_in)) < 0) {
//        std::cerr<<"Sending multicast packet failed"<<std::endl;
//        close(this->broadcastSocketHandle);
//    }
}

in_addr_t Cerios::Server::Login::getAddrFromHostname(std::string hostname, bool ipv6Prefered) {
    in_addr_t addressResult = 0;
//    int error;
//    
//    /* resolve the domain name into a list of addresses */
//    error = getaddrinfo(hostname.c_str(), NULL, NULL, &results);
//    if (error != 0) {
//        if (error == EAI_SYSTEM) {
//            perror("getaddrinfo");
//        } else {
//            fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
//        }
//        exit(EXIT_FAILURE);
//    }
//    
//    for (result = results; result != nullptr; result = result->ai_next) {
//        if (ipv6Prefered && result->ai_protocol == IPPROTO_IPV6) {
//            addressResult = ((struct sockaddr_in *) result->ai_addr)->sin_addr.s_addr;
//        }
//        
//        if (addressResult == 0) {
//            addressResult = ((struct sockaddr_in *) result->ai_addr)->sin_addr.s_addr;
//        }
//    }
//    
//    freeaddrinfo(result);
    return addressResult;
}

Cerios::Server::Login::~Login() {
//    if (groupSocketDiscription != nullptr) {
//        delete groupSocketDiscription;
//    }
//    if (this->broadcastSocketHandle > 0) {
//        shutdown(this->broadcastSocketHandle, SHUT_RDWR);
//        close(this->broadcastSocketHandle);
//    }
}