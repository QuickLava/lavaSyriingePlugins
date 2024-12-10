#ifndef LAVA_MECHANICS_HUB_H_V1
#define LAVA_MECHANICS_HUB_H_V1

#include <sy_core.h>
#include "fighterHooks.h"
#include "_cmAddonInterface.h"

namespace hubAddon
{
    enum _lineIDs
    {
        lid_WORKING_SPACE = 0,
        lid_ACTIVE_MECHANIC_P1,
        lid_ACTIVE_MECHANIC_P2,
        lid_ACTIVE_MECHANIC_P3,
        lid_ACTIVE_MECHANIC_P4,
        lid_MAGIC_SERIES_ENABLED,
        lid__COUNT
    };
    enum activeMechanicIDs
    {
        amid_NONE = 0,
        amid_AIRDODGE_CANCELS,
        amid_SLIME_CANCELS,
        amid__COUNT,
    };
    enum passiveMechanicIDs
    {
        pmid_MAGIC_SERIES,
        pmid__COUNT,
    };

    extern u32 indexBuffer[];
    const u32 fighterHipNodeID = 0x12D;

    bool populate();
    void registerHooks();
    bool getActiveMechanicEnabled(u32 playerNo, activeMechanicIDs mechanicID);
    bool getPassiveMechanicEnabled(u32 playerNo, passiveMechanicIDs mechanicID);

    float getDistanceBetween(StageObject* obj1, StageObject* obj2, bool usePrevPos);
}

#endif
