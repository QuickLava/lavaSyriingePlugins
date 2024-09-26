#include "magicSeries.h"

namespace lavaPSAExtensions
{
    char outputTag[] = "[lavaMagicSystem] ";

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

    fighterAttackWatcher::fighterAttackWatcher() : soCollisionAttackEventObserver(1) {}
    fighterAttackWatcher::~fighterAttackWatcher() {}

    void fighterAttackWatcher::addObserver(short param1, s8 param2)
    { 
        return;
    }
    bool fighterAttackWatcher::notifyEventCollisionAttackCheck(u32 flags)
    {
        return 0; 
    }

    void fighterAttackWatcher::notifyEventCollisionAttack(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser)
    {
        OSReport("%sCollisionAttack!\n", outputTag);
        soModuleEnumeration* moduleEnum = moduleAccesser->m_enumerationStart;
        if (moduleEnum != NULL)
        {
            int currSituation = moduleEnum->m_situationModule->getKind();
            int currStatus = moduleEnum->m_statusModule->getStatusKind();

            if (currSituation == 0x00)
            {
                switch (currStatus)
                {
                case SID_ATTACK_1:
                case SID_ATTACK_100:
                case SID_ATTACK_DASH:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0x7);
                    OSReport("- Enabled Jump Cancels!\n");
                }
                case SID_ATTACK_S3:
                case SID_ATTACK_HI3:
                case SID_ATTACK_LW3:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0x4);
                    OSReport("- Enabled Attack Cancels!\n");
                }
                case SID_ATTACK_S4S:
                case SID_ATTACK_LW4:
                case SID_ATTACK_HI4:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0x1);
                    OSReport("- Enabled Special Cancels!\n");
                    break;
                }
                default:
                {
                    break;
                }
                }
            }
            else if (currSituation == 0x02)
            {
                switch (currStatus)
                {
                case SID_ATTACK_AIR:
                {
                    moduleEnum->m_statusModule->enableTransitionTermGroup(0xB);
                    OSReport("- Enabled Air Special Cancels!\n");
                    break;
                }
                default:
                {
                    break;
                }
                }
            }

            
        }
    }

    void fighterAttackWatcher::subscribeToFighter(Fighter& fighterIn)
    {
        soEventManageModule* eventModule = fighterIn.m_moduleAccesser->getEventManageModule();
        if (eventModule != NULL)
        {
            int manageId = eventModule->getManageId();
            if (-1 < manageId && -1 < m_unitID) {
                if (soEventSystem::getInstance()->m_instanceManager.isContain(manageId)) {
                    soEventManager* eventManager = soEventSystem::getInstance()->getManager(manageId);
                    if (!eventManager->isNullUnit(m_unitID)) {
                        int sendId;
                        if (eventManager->isExist(m_unitID)) {
                            soEventUnitWrapper<soCollisionAttackEventObserver>* eventUnit = dynamic_cast<soEventUnitWrapper<soCollisionAttackEventObserver>*>(eventManager->getEventUnit(m_unitID));
                            if (eventUnit == NULL) {
                                sendId = -1;
                            }
                            else {
                                sendId = eventUnit->addObserverSub(static_cast<soCollisionAttackEventObserver*>(this), -1);
                                OSReport("%sRegistered Fighter [Name: %s]!\n", outputTag, fighterIn.m_taskName);
                            }
                        }
                        else {
                            sendId = -1;
                        }
                        this->m_sendID = sendId;
                        if (-1 < sendId) {
                            this->m_manageID = manageId;
                        }
                    }
                }
            }
        }
    }
}