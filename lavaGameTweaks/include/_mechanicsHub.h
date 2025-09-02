#ifndef LAVA_MECHANICS_HUB_H_V1
#define LAVA_MECHANICS_HUB_H_V1

#include <syWrapper.h>
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
        lid_HORI_WAVEDASH_TOGGLE_P1,
        lid_HORI_WAVEDASH_TOGGLE_P2,
        lid_HORI_WAVEDASH_TOGGLE_P3,
        lid_HORI_WAVEDASH_TOGGLE_P4,
        lid_ACTI_WAVEDASH_TOGGLE_P1,
        lid_ACTI_WAVEDASH_TOGGLE_P2,
        lid_ACTI_WAVEDASH_TOGGLE_P3,
        lid_ACTI_WAVEDASH_TOGGLE_P4,
        lid_BABY_DASH_TOGGLE_P1,
        lid_BABY_DASH_TOGGLE_P2,
        lid_BABY_DASH_TOGGLE_P3,
        lid_BABY_DASH_TOGGLE_P4,
        lid_WALLJUMP_BUTTON_TOGGLE_P1,
        lid_WALLJUMP_BUTTON_TOGGLE_P2,
        lid_WALLJUMP_BUTTON_TOGGLE_P3,
        lid_WALLJUMP_BUTTON_TOGGLE_P4,
        lid_WALLJUMP_FROM_SPECIAL_TOGGLE_P1,
        lid_WALLJUMP_FROM_SPECIAL_TOGGLE_P2,
        lid_WALLJUMP_FROM_SPECIAL_TOGGLE_P3,
        lid_WALLJUMP_FROM_SPECIAL_TOGGLE_P4,
        lid_WALLJUMP_ONCE_PER_AIR_P1,
        lid_WALLJUMP_ONCE_PER_AIR_P2,
        lid_WALLJUMP_ONCE_PER_AIR_P3,
        lid_WALLJUMP_ONCE_PER_AIR_P4,
        lid_TILT_CANCELS_P1,
        lid_TILT_CANCELS_P2,
        lid_TILT_CANCELS_P3,
        lid_TILT_CANCELS_P4,
        lid_SHIELD_SIZE_LOCK_P1,
        lid_SHIELD_SIZE_LOCK_P2,
        lid_SHIELD_SIZE_LOCK_P3,
        lid_SHIELD_SIZE_LOCK_P4,
        lid_SHIELD_BREAK_REDUCTION_P1,
        lid_SHIELD_BREAK_REDUCTION_P2,
        lid_SHIELD_BREAK_REDUCTION_P3,
        lid_SHIELD_BREAK_REDUCTION_P4,
        lid_SHIELD_PARRY_P1,
        lid_SHIELD_PARRY_P2,
        lid_SHIELD_PARRY_P3,
        lid_SHIELD_PARRY_P4,
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
        pmid_HORI_WAVEDASH,
        pmid_ACTI_WAVEDASH,
        pmid_BABY_DASH,
        pmid_WALLJUMP_BUTTON,
        pmid_WALLJUMP_FROM_SPECIAL,
        pmid_WALLJUMP_ONCE_PER_AIR,
        pmid_TILT_CANCELS,
        pmid_SHIELD_SIZE_LOCK,
        pmid_SHIELD_BREAK_REDUCTION,
        pmid_SHIELD_PARRY,
        pmid__COUNT,
    };

    extern u32 indexBuffer[];

    bool populate();
    void registerHooks();
    bool getActiveMechanicEnabled(u32 playerNo, activeMechanicIDs mechanicID);
    bool getPassiveMechanicEnabled(u32 playerNo, passiveMechanicIDs mechanicID);
    bool getActiveMechanicEnabledDiff(u32 playerNo, activeMechanicIDs mechanicID);
    bool getPassiveMechanicEnabledDiff(u32 playerNo, passiveMechanicIDs mechanicID);
}

#endif
