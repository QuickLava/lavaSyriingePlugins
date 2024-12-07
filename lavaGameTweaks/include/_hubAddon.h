#ifndef LAVA_HUB_ADDON_H_V1
#define LAVA_HUB_ADDON_H_V1

#include <sy_core.h>
#include "_cmAddonInterface.h"

namespace hubAddon
{
    enum _lineIDs
    {
        lid_WORKING_SPACE = 0,
        lid_ACTIVE_MECHANIC,
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

    extern u32 indexBuffer[];

    void populate();
    u32 getActiveMechanic();
}

#endif
