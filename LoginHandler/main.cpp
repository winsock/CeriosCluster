//
//  main.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include <iostream>
#include "LoginServer.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    Cerios::Server::Login *loginServer = new Cerios::Server::Login(25565, 1337, false);
    loginServer->init();
    loginServer->listen();
    delete loginServer;
    return EXIT_SUCCESS;
}
