#include "transitionListener.h"

namespace transitionListener
{
    char outputTag[] = "[transitionListener] ";
    
    void onStatusChangeCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            u32 currStatus = moduleEnum->m_statusModule->getStatusKind();
            u32 prevStatus = moduleEnum->m_statusModule->getPrevStatusKind(0);
            if (currStatus != prevStatus)
            {
                soTransitionInfo* lastTransition = ((soStatusModuleImpl*)moduleEnum->m_statusModule)->m_transitionModule->getLastTransitionInfo();
                OSReport_N("%sP%d: Status[0x%04X -> 0x%04X], Transition[0x%04X_0x%04X]\n", outputTag, fighterPlayerNo,
                    prevStatus, currStatus, lastTransition->m_unitId, lastTransition->m_groupId);
            }
        }
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnStatusChangeCB = (fighterHooks::FighterOnStatusChangeCB)onStatusChangeCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}