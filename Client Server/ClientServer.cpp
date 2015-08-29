//
//  ClientServer.cpp
//  Client Server
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "ClientServer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>

void Cerios::Server::Client::init() {
    /* Create a datagram socket on which to receive. */
    this->clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->clientSocket < 0) {
        perror("Opening datagram socket error");
        exit(EXIT_FAILURE);
    }
    
    /* Enable SO_REUSEADDR to allow multiple instances of this */
    /* application to receive copies of the multicast datagrams. */
    int reuse = 1;
    if(setsockopt(this->clientSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        perror("Setting SO_REUSEADDR error");
        close(this->clientSocket);
        exit(EXIT_FAILURE);
    }
    
    /* Bind to the proper port number with the IP address */
    /* specified as INADDR_ANY. */
    this->localSocketAddress = new struct sockaddr_in;
    memset(this->localSocketAddress, 0, sizeof(struct sockaddr_in));
    this->localSocketAddress->sin_family = AF_INET;
    this->localSocketAddress->sin_port = htons(4321);
    this->localSocketAddress->sin_addr.s_addr = INADDR_ANY;
    if(bind(this->clientSocket, (struct sockaddr*)this->localSocketAddress, sizeof(struct sockaddr_in))) {
        perror("Binding datagram socket error");
        close(this->clientSocket);
        exit(EXIT_FAILURE);
    }
    
    /* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
    /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
    /* called for each local interface over which the multicast */
    /* datagrams are to be received. */
    
    struct ip_mreq group;
    group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
    group.imr_interface.s_addr = inet_addr("127.0.0.1");
    if(setsockopt(this->clientSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
        perror("Adding multicast group error");
        close(this->clientSocket);
        exit(EXIT_FAILURE);
    }
}

void Cerios::Server::Client::listen() {
    /* Read from the socket. */
    char databuf[1024];
    int datalen = sizeof(databuf);
    if(read(this->clientSocket, databuf, datalen) < 0) {
        perror("Reading datagram message error");
        close(this->clientSocket);
        exit(EXIT_FAILURE);
    } else {
        printf("Reading datagram message...OK.\n");
        printf("The message from multicast server is: \"%s\"\n", databuf);
    }
}

Cerios::Server::Client::~Client() {
    if (localSocketAddress != nullptr) {
        delete localSocketAddress;
    }
    if (this->clientSocket > 0) {
        shutdown(this->clientSocket, SHUT_RDWR);
        close(this->clientSocket);
    }
}