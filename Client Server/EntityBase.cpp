//
//  EntityBase.cpp
//  Client Server
//
//  Created by Andrew Querol on 9/4/15.
//  Copyright © 2015 Andrew Querol. All rights reserved.
//

#include "EntityBase.hpp"

std::uint64_t Cerios::Server::Entity::EntityBase::getEntityId() {
    return this->entityId;
}