#include "transitionListener.h"

namespace transitionListener
{
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
                soTransitionInfo* lastTransition = transitionModule->getLastTransitionInfo();
                OSReport_N("%sP%d: Status[0x%04X -> 0x%04X], Transition[0x%04X-0x%04X:0x%04X]\n", outputTag, fighterPlayerNo,
                    prevStatus, currStatus, lastTransition->m_unitId, lastTransition->m_groupId, lastTransition->_unk08);
                const soGeneralTermManager* termManager = &g_soGeneralTermManager;
                soArray<soTransitionTermGroup>* groupVec = transitionModule->m_transitionTermGroupArray;
                const u32 groupCount = groupVec->size();
                OSReport_N("%sP%d TransModule Component Log (Group Count: %02d):\n", outputTag, fighterPlayerNo, groupCount);
                for (u32 i = 0; i < groupCount; i++)
                {
                    soTransitionTermGroup& currGroup = groupVec->at(i);
                    OSReport_N("%s- Group %02X [%04X] @ %08X: Unk = %08X ", outputTag, i, currGroup.m_unitID, &currGroup, *((u32*)currGroup._unk00));

                    soArray<soInstanceUnitFullProperty<soTransitionTerm> >* termVecPtr = currGroup.m_transitionTermInstanceManager.m_array;
                    const u32 termCount = termVecPtr->size();
                    OSReport_N("(InstanceMgr: Size = %02d, Capacity = %02d)\n", termCount, termVecPtr->capacity());
                    for (u32 u = 0; u < termCount; u++)
                    {
                        soInstanceUnitFullProperty<soTransitionTerm>& currTermProp = termVecPtr->at(u);
                        soTransitionTerm* currTermPtr = &currTermProp.m_element;

                        OSReport_N("%s   [%02X] : [%02X-%5d-%03X-%02X] : ", outputTag, u,
                            currTermProp.m_attribute, currTermProp.m_id, currTermPtr->m_targetKind, currTermPtr->m_flags);
                        int currIndex = currTermPtr->m_generalTermIndex;
                        while (currIndex > 0)
                        {
                            soGeneralTerm* currGeneralTerm = termManager->m_generalTerms2 + currIndex;
                            const u32 argCount = currGeneralTerm->m_animCmdTable.size();
                            acCmdArgConv* currArg = &currGeneralTerm->m_animCmdTable.at(0);
                            OSReport_N("<%03X@%08X", currIndex, currArg);
                            for (u32 y = 0; y < argCount; y++)
                            {
                                u32 data = currArg->data;
                                u32 argType = currArg->argType;
                                OSReport_N(":%1X-", currArg->argType);
                                switch (argType)
                                {
                                    case AnimCmd_Arg_Type_Variable:
                                    {
                                        OSReport_N("%04d", data);
                                        break;
                                    }
                                    case AnimCmd_Arg_Type_Requirement:
                                    {
                                        if (data & 0x80000000)
                                        {
                                            data &= ~0x80000000;
                                            OSReport_N("!");
                                        }
                                        OSReport_N("%04X", data);
                                        break;
                                    }
                                    default:
                                    {
                                        OSReport_N("%04X", data);
                                    }
                                }
                                currArg++;
                            }
                            OSReport_N("> ");

                            currIndex = termManager->m_indices2[currIndex];
                        }
                        OSReport_N("\n");
                    }
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