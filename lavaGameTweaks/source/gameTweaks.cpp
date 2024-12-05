#include "fighterHooks.h"
#include "magicSeries.h"
#include "meleeGameSetFreeze.h"
#include "reflectOnHit.h"
#include "airdodgeCancels.h"
#include "slimeCancels.h"

namespace lavaGameTweaks
{
    void Init()
    {
        // Note: 0x8070AA14 is SORA_MELEE base address
        fighterHooks::registerFighterHooks();
        //magicSeries::registerHooks();
        //meleeFreeze::registerHooks();
        //reflectOnHit::registerHooks();
        //airdodgeCancels::registerHooks();
        slimeCancels::registerHooks();
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}