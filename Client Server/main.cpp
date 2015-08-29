//
//  main.cpp
//  Client Server
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include <iostream>
#include "ClientServer.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    Cerios::Server::Client *client = new Cerios::Server::Client;
    client->init();
    client->listen();
    delete client;
    return EXIT_SUCCESS;
}
