#include "magicSeries.h"
#include <ft/fighter.h>

namespace magicSeries
{
    const char outputTag[] = "[lavaMagicSeries] ";
    enum actionStatusIDs
    {
        ASID_ATTACK_S3 = 0x2764,
        ASID_ATTACK_HI3 = 0x2765,
        ASID_ATTACK_LW3 = 0x2766,
        ASID_ATTACK_S4_START = 0x2768,
        ASID_ATTACK_HI4_START = 0x276B,
        ASID_ATTACK_LW4_START = 0x276E,
    };

    void magicSeriesCallback(Fighter* attacker, StageObject* target, float damage)
    {
        OSReport_N("%sAttack Detected!\n", outputTag);
        soModuleEnumeration* moduleEnum = &attacker->m_moduleAccesser->m_moduleEnumeration;
        if (moduleEnum != NULL)
        {
            int currStatus = moduleEnum->m_statusModule->getStatusKind();
            soTransitionModule* transitionModule = ((soStatusModuleImpl*)moduleEnum->m_statusModule)->m_transitionModule;
            switch (currStatus)
            {
            case Fighter::Status_Attack: case Fighter::Status_Attack_100: case Fighter::Status_Attack_Dash: 
            case Fighter::Status_Attack_S3: case Fighter::Status_Attack_Hi3: case Fighter::Status_Attack_Lw3:
            {
                transitionModule->enableTermGroup(0x4);
                transitionModule->unableTermAll(0x4);
                if (currStatus <= Fighter::Status_Attack_Dash)
                {
                    transitionModule->enableTerm(ASID_ATTACK_S3, 0x4);
                    transitionModule->enableTerm(ASID_ATTACK_HI3, 0x4);
                    transitionModule->enableTerm(ASID_ATTACK_LW3, 0x4);
                }
                transitionModule->enableTerm(ASID_ATTACK_S4_START, 0x4);
                transitionModule->enableTerm(ASID_ATTACK_HI4_START, 0x4);
                transitionModule->enableTerm(ASID_ATTACK_LW4_START, 0x4);

                OSReport_N("- Enabled Attack Cancels!\n");
            }
            case Fighter::Status_Attack_S4: case Fighter::Status_Attack_Lw4: case Fighter::Status_Attack_Hi4: case Fighter::Status_Attack_Air:
            {
                if (moduleEnum->m_situationModule->getKind() == 0x00)
                {
                    transitionModule->enableTermGroup(0x1);
                }
                else
                {
                    transitionModule->enableTermGroup(0xB);
                }
                OSReport_N("- Enabled Special Cancels!\n");
                break;
            }
            }
        }
    }

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerOnAttackCallback(magicSeriesCallback);
    }
}