#ifndef LAVA_MAGIC_SERIES_H_V1
#define LAVA_MAGIC_SERIES_H_V1

#include <cstdlib>
#include "fighterHooks.h"

namespace lavaPSAExtensions
{
    void magicSeriesCallback(Fighter* attacker, StageObject* target, float power, soCollisionLog* collisionLog);
}

#endif
