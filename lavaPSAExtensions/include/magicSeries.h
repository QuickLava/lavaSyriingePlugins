#ifndef LAVA_MAGIC_SERIES_H_V1
#define LAVA_MAGIC_SERIES_H_V1

#include <cstdlib>
#include <so/event/so_event_observer.h>
#include "fighterHooks.h"

namespace lavaPSAExtensions
{
    void magicSeriesCallback(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser);
}

#endif
