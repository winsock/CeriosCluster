//
//  main.cpp
//  Client Server
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include <iostream>
#include <memory>

#include "ClientServer.hpp"

int main(int argc, const char * argv[]) {
    Cerios::Server::ClientServer client(1337, false);
    client.init();
    client.listen();
    return EXIT_SUCCESS;
}
