#include "magicSeries.h"
#include <ft/fighter.h>

using namespace codeMenu;
namespace magicSeries
{
    char outputTag[] = "[magicSeries] ";

    void magicSeriesCallback(Fighter* attacker, StageObject* target, float damage)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_MAGIC_SERIES))
        {
            OSReport_N("%sAttack Detected!\n", outputTag);

            soModuleEnumeration* moduleEnum = &attacker->m_moduleAccesser->m_moduleEnumeration;
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
                    transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_S3, 0x4);
                    transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_Hi3, 0x4);
                    transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_Lw3, 0x4);
                }
                transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_S4_Start, 0x4);
                transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_Hi4_Start, 0x4);
                transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_Lw4_Start, 0x4);

                OSReport_N("- Enabled Attack Cancels!\n");
            }
            case Fighter::Status_Attack_S4: case Fighter::Status_Attack_Lw4: case Fighter::Status_Attack_Hi4: case Fighter::Status_Attack_Air:
            {
                if (moduleEnum->m_situationModule->getKind() == 0x00)
                {
                    transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Ground_Special);
                }
                else
                {
                    transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Special);
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