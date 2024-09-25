#include "magicSeries.h"

namespace lavaPSAExtensions
{
    char outputTag[] = "[lavaMagicSystem] ";

    fighterAttackWatcher::fighterAttackWatcher(Fighter& fighterIn) : soCollisionAttackEventObserver(1)
    {
        soEventManageModule* eventModule = fighterIn.m_moduleAccesser->getEventManageModule();
        if (eventModule != NULL)
        {
            setupObserver(eventModule->getManageId());
            OSReport("%sRegistered Fighter [Name: %s]!\n", outputTag, fighterIn.m_taskName);
        }
    }
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


    }
}