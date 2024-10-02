#include "magicSeries.h"

namespace magicSeries
{
    const char outputTag[] = "[lavaMagicSeries] ";
    enum statusIDs
    {
        SID_ATTACK_1 = 0x24,
        SID_ATTACK_100 = 0x25,
        SID_ATTACK_DASH = 0x26,
        SID_ATTACK_S3 = 0x27,
        SID_ATTACK_HI3 = 0x28,
        SID_ATTACK_LW3 = 0x29,
        SID_ATTACK_S4S = 0x2C,
        SID_ATTACK_LW4 = 0x2F,
        SID_ATTACK_HI4 = 0x32,
        SID_ATTACK_AIR = 0x33,
    };

    void magicSeriesCallback(Fighter* attacker, StageObject* target, float damage)
    {
        OSReport_N("%sAttack Detected!\n", outputTag);
        soModuleEnumeration* moduleEnum = &attacker->m_moduleAccesser->m_moduleEnumeration;
        if (moduleEnum != NULL)
        {
            int currSituation = moduleEnum->m_situationModule->getKind();
            int currStatus = moduleEnum->m_statusModule->getStatusKind();

            if (currSituation == 0x00)
            {
                switch (currStatus)
                {
                case SID_ATTACK_1: case SID_ATTACK_100: case SID_ATTACK_DASH:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0x7);
                    OSReport_N("- Enabled Jump Cancels!\n");
                }
                case SID_ATTACK_S3: case SID_ATTACK_HI3: case SID_ATTACK_LW3:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0x4);
                    OSReport_N("- Enabled Attack Cancels!\n");
                }
                case SID_ATTACK_S4S: case SID_ATTACK_LW4: case SID_ATTACK_HI4:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0x1);
                    OSReport_N("- Enabled Special Cancels!\n");
                    break;
                }
                }
            }
            else if (currSituation == 0x02 && currStatus == SID_ATTACK_AIR)
            {
                moduleEnum->m_statusModule->enableTransitionTermGroup(0xB);
                OSReport_N("- Enabled Air Special Cancels!\n");
            }
        }
    }
    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerOnAttackCallback(magicSeriesCallback);
    }
}