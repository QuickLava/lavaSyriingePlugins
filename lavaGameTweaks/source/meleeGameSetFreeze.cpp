#include "meleeGameSetFreeze.h"

namespace meleeFreeze
{
    bool freezeTriggered = 0;

    void meleeFreezeCB()
    {
        typedef void (*SetHitStopFramePtr)(void*, int, int);

        OSReport_N("MeleeFreeze!\n");

        GameGlobal* gameGlobalPtr = *fighterHooks::g_GameGlobalPtrAddr;
        gameGlobalPtr->m_stageData->m_motionRatio = 0.0f;

        ftManager* fighterManager = *fighterHooks::g_ftManagerPtrAddr;
        u32 currFighterEntryID = -1;
        Fighter* currFighterPtr = NULL;
        for (int i = 0; i < fighterHooks::maxFighterCount; i++)
        {
            currFighterEntryID = fighterManager->getEntryId(i);
            if (currFighterEntryID != -1)
            {
                currFighterPtr = fighterManager->getFighter(currFighterEntryID, -1);
                void* stopModule = currFighterPtr->m_moduleAccesser->m_moduleEnumeration.m_stopModule;
                SetHitStopFramePtr func = (SetHitStopFramePtr)((*(int**)stopModule)[0x12]);
                func(stopModule, 6000, 0);
            }
        }
    }

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerMeleeOnGameSetCallback(meleeFreezeCB);
    }
}