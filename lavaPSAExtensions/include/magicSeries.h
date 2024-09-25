#ifndef LAVA_MAGIC_SERIES_H_V1
#define LAVA_MAGIC_SERIES_H_V1

#include <cstdlib>
#include <so/event/so_event_observer.h>
#include <ft/fighter.h>

namespace lavaPSAExtensions
{
    class fighterAttackWatcher : public soCollisionAttackEventObserver
    {
    public:
        fighterAttackWatcher(Fighter& fighterIn);
        virtual ~fighterAttackWatcher();

        virtual void addObserver(short param1, s8 param2);
        virtual bool notifyEventCollisionAttackCheck(u32 flags);

        virtual void notifyEventCollisionAttack(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser);
    };

    void registerMagicSeriesHooks();
}

#endif
