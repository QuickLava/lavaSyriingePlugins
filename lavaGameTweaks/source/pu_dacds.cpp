#include "pu_dacds.h"

namespace puDACDS
{
    char outputTag[] = "[DACDS] ";
    const u32 momentumVar = 0x21000005;
    const u32 dacusFlagVar = 0x22000020;

    u8 prevUpdateActionWasDashAttack = 0x00;

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            u32 currStatus = statusModule->getStatusKind();
            u32 prevStatus = statusModule->getPrevStatusKind(0);
            float currFrame = moduleAccesser->m_enumerationStart->m_motionModule->getFrame();

            u32 playerBit = 1 << fighterPlayerNo;
            u32 prevUpdateActionWasDashAttackTemp = prevUpdateActionWasDashAttack;

            if (currStatus == Fighter::Status_Attack_Dash)
            {
                soTransitionModule* transitionModule = statusModule->m_transitionModule;
                if (currFrame <= 3.0f || moduleAccesser->m_enumerationStart->m_collisionAttackModule->isInflictStatus())
                {
                    transitionModule->enableTermGroup(Fighter::Status_Transition_Group_Chk_Ground_Attack);
                    transitionModule->unableTermAll(Fighter::Status_Transition_Group_Chk_Ground_Attack);
                    transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_Hi4_Start, Fighter::Status_Transition_Group_Chk_Ground_Attack);
                    transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_Lw4_Start, Fighter::Status_Transition_Group_Chk_Ground_Attack);
                }
                else
                {
                    transitionModule->unableTermGroup(Fighter::Status_Transition_Group_Chk_Ground_Attack);
                }
            }
            else if (!workManageModule->isFlag(dacusFlagVar) && (prevUpdateActionWasDashAttackTemp & playerBit)
                && (prevStatus == Fighter::Status_Attack_Dash && (currStatus == Fighter::Status_Attack_Hi4_Start || currStatus == Fighter::Status_Attack_Lw4_Start)))
            {
                workManageModule->onFlag(dacusFlagVar);
                Vec3f newSpeed(workManageModule->getFloat(momentumVar), 0.0f, 0.0f);
                moduleAccesser->m_enumerationStart->m_kineticModule->addSpeed(&newSpeed, moduleAccesser);
                OSReport_N("%sDACU/DS Activated!\n", outputTag);
            }

            prevUpdateActionWasDashAttackTemp &= ~playerBit;
            if (currStatus == Fighter::Status_Attack_Dash)
            {
                prevUpdateActionWasDashAttackTemp |= playerBit;
            }
            prevUpdateActionWasDashAttack = prevUpdateActionWasDashAttackTemp;
        }
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}