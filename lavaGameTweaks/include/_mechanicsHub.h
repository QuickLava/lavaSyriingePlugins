#ifndef LAVA_MECHANICS_HUB_H_V1
#define LAVA_MECHANICS_HUB_H_V1

#include <sy_core.h>
#include "fighterHooks.h"
#include "fighterMeters.h"
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
        pmid__COUNT,
    };
    
    extern u32 indexBuffer[];

    bool populate();
    void registerHooks();
    bool getActiveMechanicEnabled(u32 playerNo, activeMechanicIDs mechanicID);
    bool getPassiveMechanicEnabled(u32 playerNo, passiveMechanicIDs mechanicID);
}

// Utility Functions and Constants
namespace mechUtil
{
    // These are used with soModelModule::getCorrectBoneID(), they map consistent values to certain other bones!
    enum SpecialBoneIDs
    {
        sbid_ThrowN = 300,
        sbid_HipN,
        sbid_XRotN,
        sbid_YRotN,
        sbid_TransN,
        sbid_LHaveN,
        sbid_LShoulderN,
        sbid_RShoulderN,
        sbid_RShoulderN2, // Unsure why this and the above are the same.
        sbid_WaistN,
    };
    const u32 gfxRootBoneID = sbid_XRotN;
    const float radianConvConstant = 3.141593f * 0.005555f;
    extern Vec3f zeroVec;
    extern Vec3f gfxFaceScreenRotVec;
    extern Vec3f gfxFlattenSclVec;
    const u32 allTauntPadMask =
        INPUT_PAD_BUTTON_MASK_APPEAL_HI | INPUT_PAD_BUTTON_MASK_APPEAL_S | INPUT_PAD_BUTTON_MASK_APPEAL_LW |
        INPUT_PAD_BUTTON_MASK_APPEAL_S_L | INPUT_PAD_BUTTON_MASK_APPEAL_S_R;

    enum meterGainAnnouncerCond
    {
        mgac_ON_STOCK_GAIN = 0x1,
        mgac_ON_STOCK_LOSS = 0x2,
        mgac_ALWAYS = 0x3,
    };
    // Returns change in Stock Count
    int doMeterGain(Fighter* fighterIn, float meterIn, EfID meterGainGraphic, float graphicScale, meterGainAnnouncerCond announcerClipCond);

    float getDistanceBetween(StageObject* obj1, StageObject* obj2, bool usePrevPos);
    u32 reqCenteredGraphic(StageObject* obj1, EfID effectID, float scale, bool follow);
    u32 playSE(StageObject* objectIn, SndID soundID);
    bool isAttackingStatusKind(u32 statusKind);
    bool isDamageStatusKind(u32 statusKind);
    void initDefaultHitboxData(soCollisionAttackData* attackDataIn);
    float currAnimProgress(StageObject* objectIn);
}

#endif
