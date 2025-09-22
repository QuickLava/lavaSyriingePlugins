#include "transitionListener.h"

namespace transitionListener
{
    const u32 g_GeneralTermCache = 0x80B84F08;
    char outputTag[] = "[transitionListener] ";
    
    void onStatusChangeCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            u32 currStatus = statusModule->getStatusKind();
            u32 prevStatus = statusModule->getPrevStatusKind(0);
            if (currStatus != prevStatus)
            {
                soTransitionModuleImpl* transitionModule = (soTransitionModuleImpl*)statusModule->m_transitionModule;
                /*u16 shortBuffer[2];
                int transitionBuffer[5];
                shortBuffer[0] = 0;
                transitionBuffer[0] = -1;
                transitionModule->checkEstablish(moduleAccesser, transitionBuffer, -1, shortBuffer, g_GeneralTermCache);
                OSReport_N("%sBuffer1: {%08X, %08X, %08X, %08X, %08X}\n", outputTag,
                    transitionBuffer[0], transitionBuffer[1], transitionBuffer[2], transitionBuffer[3], transitionBuffer[4]);
                OSReport_N("%sBuffer2: {%04X, %04X}\n", outputTag, shortBuffer[0], shortBuffer[1]);*/
                soTransitionInfo* lastTransition = transitionModule->getLastTransitionInfo();
                OSReport_N("%sP%d: Status[0x%04X -> 0x%04X], Transition[0x%04X_0x%04X]\n", outputTag, fighterPlayerNo,
                    prevStatus, currStatus, lastTransition->m_unitId, lastTransition->m_groupId);

                soTransitionModuleImpl::tdef_GroupArray groupVec = transitionModule->m_transitionTermGroupArray;
                const u32 groupCount = groupVec->size();
                OSReport_N("%sP%d TransModule Component Log (Group Count: %02d):\n", outputTag, fighterPlayerNo, groupCount);
                for (u32 i = 0; i < groupCount; i++)
                {
                    soTransitionTermGroup& currGroup = groupVec->at(i);
                    OSReport_N("%s- Group %02X [%04X] @ %08X: Unk = %08X\n", outputTag, i, currGroup.m_unitID, &currGroup, *((u32*)currGroup._unk00));

                    soTransitionTermGroup::tdef_InstanceMgr instanceMgr = currGroup.m_transitionTermInstanceManager;

                    soTransitionTermGroup::tdef_InstanceMgr::tdef_ArrayVec instanceMgrVecPtr = instanceMgr.m_arrayVector;
                    const u32 termCount = instanceMgrVecPtr->size();
                    OSReport_N("%s  InstanceMgr: Size = %02d, Capacity = %02d", outputTag, termCount, instanceMgrVecPtr->capacity());
                    for (u32 u = 0; u < termCount; u++)
                    {
                        if ((u % 4) == 0)
                        {
                            OSReport_N("\n%s   [%02X) : ", outputTag, u);
                        }
                        soInstanceUnitFullProperty<soTransitionTerm>& currTerm = instanceMgrVecPtr->at(u);
                        soTransitionTerm* currTermPtr = &currTerm.m_element;
                        OSReport_N("[%02X_%5d_%02X-%04X_%04X] ", 
                            currTerm.m_attribute, currTerm.m_id,
                            currTermPtr->m_targetKind, *((u16*)currTermPtr->_unk00), *((u16*)currTermPtr->_unk06));
                    }
                    OSReport_N("\n");
                }
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