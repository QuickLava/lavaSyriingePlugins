#include "transitionListener.h"

namespace transitionListener
{
    char outputTag[] = "[transitionListener] ";
    
    u32 prevFrameStatusBak[fighterHooks::maxFighterCount] = {};
    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            u32 currStatus = moduleEnum->m_statusModule->getStatusKind();
            u32 prevFrameStatus = prevFrameStatusBak[fighterPlayerNo];
            if (currStatus != prevFrameStatus)
            {
                soTransitionInfo* lastTransition = ((soStatusModuleImpl*)moduleEnum->m_statusModule)->m_transitionModule->getLastTransitionInfo();
                OSReport_N("%sP%d: Status[0x%04X -> 0x%04X], Transition[0x%04X_0x%04X]\n", outputTag, fighterPlayerNo,
                    prevFrameStatus, currStatus, lastTransition->m_unitId, lastTransition->m_groupId);
            }
            prevFrameStatusBak[fighterPlayerNo] = currStatus;
        }
    }

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerOnUpdateCallback(onUpdateCallback);
    }
}