#include "magicSeries.h"

namespace lavaPSAExtensions
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

    void magicSeriesCallback(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser, StageObject* targetObject)
    {
        OSReport("%sAttack Detected!\n", outputTag);
        soModuleEnumeration* moduleEnum = moduleAccesser->m_enumerationStart;
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
                    OSReport("- Enabled Jump Cancels!\n");
                }
                case SID_ATTACK_S3: case SID_ATTACK_HI3: case SID_ATTACK_LW3:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0x4);
                    OSReport("- Enabled Attack Cancels!\n");
                }
                case SID_ATTACK_S4S: case SID_ATTACK_LW4: case SID_ATTACK_HI4:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0x1);
                    OSReport("- Enabled Special Cancels!\n");
                    break;
                }
                }
            }
            else if (currSituation == 0x02 && currStatus == SID_ATTACK_AIR)
            {
                moduleEnum->m_statusModule->enableTransitionTermGroup(0xB);
                OSReport("- Enabled Air Special Cancels!\n");
            }
        }
    }
}