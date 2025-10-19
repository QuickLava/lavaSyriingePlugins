#include "meleeGameSetFreeze.h"

namespace meleeFreeze
{
    const char outputTag[] = "[meleeFreeze] ";

    void meleeFreezeCB()
    {
        typedef void (*SetHitStopFramePtr)(void*, int, int);

        OSReport_N("%sApplied Freeze\n", outputTag);

        g_GameGlobal->m_stageData->m_motionRatio = 0.0f;

        ftManager* fighterManager = g_ftManager;
        u32 currFighterEntryID = -1;
        Fighter* currFighterPtr = NULL;
        for (int i = 0; i < fighterHooks::maxFighterCount; i++)
        {
            currFighterEntryID = fighterManager->getEntryId(i);
            if (currFighterEntryID != -1)
            {
                currFighterPtr = fighterManager->getFighter(currFighterEntryID, -1);
                currFighterPtr->m_moduleAccesser->m_moduleEnumeration.m_stopModule->setHitStopFrame(6000, 0);
                // 00ffff02
                // 8087C734
            }
        }
        //g_GameGlobal->setSlowRate(1);
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_MeleeOnGameSetCB = (fighterHooks::MeleeOnGameSetCB)meleeFreezeCB,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}