//
//  ClientServer.hpp
//  Client Server
//
//  Created by Andrew Querol on 8/26/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef ClientServer_hpp
#define ClientServer_hpp
struct sockaddr_in;
namespace Cerios { namespace Server {
    class Client {
    private:
        int clientSocket = -1;
        sockaddr_in *localSocketAddress = nullptr;
    public:
        void init();
        void listen();
        ~Client();
    };
}}

#endif /* ClientServer_hpp */
