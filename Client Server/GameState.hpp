//
//  GameState.hpp
//  Client Server
//
//  Created by Andrew Querol on 9/4/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef GameState_hpp
#define GameState_hpp

#include <memory>
#include <cstdint>
#include <string>

#include "Bitfield.hpp"
#include "EntityPlayer.hpp"

namespace Cerios { namespace Server {
    typedef union {
        BitField<0, 3> gamemode;
        BitField<3> hardcoreFlag; // Why... Mojang
        BitField<0, 4> gamemodeByte;
    } GameMode;
    
    enum class Difficulty {
        PEACEFUL = 0,
        EASY = 1,
        NORMAL = 2,
        HARD = 3
    };
    
    class GameState {
    private:
        std::string clientId;
        std::shared_ptr<Entity::EntityPlayer> playerEntity;
        GameMode gameMode;
        std::uint8_t dimension;
        Difficulty difficulty;
        bool showReducedDebugInfo = false;
    };
}}
#endif /* GameState_hpp */
