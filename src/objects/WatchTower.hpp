/***
    Tent building for Tribalia

    Copyright (C) 2016 Arthur M
***/

#include "../logic/Building.hpp"
#include "ObjectMacros.h"

#ifndef WATCHTOWER_H
#define WATCHTOWER_H

namespace Tribalia {

/*  A watch tower is a basic watch building */
class WatchTower : public Logic::Building {
private:
    static const int TID = 003;
public:
    WatchTower(int oid, float x, float y, float z);

    /* Called on object initialization */
    virtual bool Initialize();

    /* Called on each engine loop, when an action is performed */
    virtual bool DoAction(void);
};

}



#endif /* end of include guard: WATCHTOWER_H */