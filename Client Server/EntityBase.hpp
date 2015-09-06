//
//  EntityBase.hpp
//  Client Server
//
//  Created by Andrew Querol on 9/4/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#ifndef EntityBase_hpp
#define EntityBase_hpp

#include <cstdint>

namespace Cerios { namespace Server { namespace Entity {
    class EntityBase {
    protected:
        std::uint64_t entityId;
    public:
        std::uint64_t getEntityId();
    };
}}}

#endif /* EntityBase_hpp */
