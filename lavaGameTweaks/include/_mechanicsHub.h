#ifndef LAVA_MECHANICS_HUB_H_V1
#define LAVA_MECHANICS_HUB_H_V1

#include <sy_core.h>
#include "fighterHooks.h"
#include "fighterMeters.h"
#include "_mechanicsUtil.h"
#include "_cmAddonInterface.h"

namespace mechHub
{
    enum _lineIDs
    {
        lid_WORKING_SPACE = 0,
        lid_ACTIVE_MECHANIC_P1,
        lid_ACTIVE_MECHANIC_P2,
        lid_ACTIVE_MECHANIC_P3,
        lid_ACTIVE_MECHANIC_P4,
        lid_MAGIC_SERIES_TOGGLE_P1,
        lid_MAGIC_SERIES_TOGGLE_P2,
        lid_MAGIC_SERIES_TOGGLE_P3,
        lid_MAGIC_SERIES_TOGGLE_P4,
        lid_FINAL_SMASH_METER_TOGGLE_P1,
        lid_FINAL_SMASH_METER_TOGGLE_P2,
        lid_FINAL_SMASH_METER_TOGGLE_P3,
        lid_FINAL_SMASH_METER_TOGGLE_P4,
        lid_FOCUS_ATTACKS_TOGGLE_P1,
        lid_FOCUS_ATTACKS_TOGGLE_P2,
        lid_FOCUS_ATTACKS_TOGGLE_P3,
        lid_FOCUS_ATTACKS_TOGGLE_P4,
        lid_SQUAT_DODGE_TOGGLE_P1,
        lid_SQUAT_DODGE_TOGGLE_P2,
        lid_SQUAT_DODGE_TOGGLE_P3,
        lid_SQUAT_DODGE_TOGGLE_P4,
        lid__COUNT
    };
    enum activeMechanicIDs
    {
        amid_NONE = 0,
        amid_AIRDODGE_CANCELS,
        amid_SLIME_CANCELS,
        amid_ROCKET_BURST,
        amid__COUNT,
    };
    enum passiveMechanicIDs
    {
        pmid_MAGIC_SERIES,
        pmid_FINAL_SMASH_METER,
        pmid_FOCUS_ATTACKS,
        pmid_SQUAT_DODGE,
        pmid__COUNT,
    };

    extern u32 indexBuffer[];

    bool populate();
    void registerHooks();
    bool getActiveMechanicEnabled(u32 playerNo, activeMechanicIDs mechanicID);
    bool getPassiveMechanicEnabled(u32 playerNo, passiveMechanicIDs mechanicID);
}

#endif
