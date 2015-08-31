//
//  main.cpp
//  LoginHandler
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include <iostream>
#include <memory>
#include "LoginServer.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
    std::shared_ptr<Cerios::Server::Login> loginServer(new Cerios::Server::Login(25565, 1337, false));
    loginServer->init();
    loginServer->listen();
    return EXIT_SUCCESS;
}
