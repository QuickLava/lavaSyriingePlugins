#include "fighterHooks.h"
#include "magicSeries.h"
#include "meleeGameSetFreeze.h"
#include "reflectOnHit.h"
#include "airdodgeCancels.h"
#include "slimeCancels.h"
#include "_mechanicsHub.h"

// Note: 0x8070AA14 is SORA_MELEE base address
namespace lavaGameTweaks
{
    void Init()
    {
        fighterHooks::registerFighterHooks();
        mechHub::populate();
        mechHub::registerHooks();
        airdodgeCancels::registerHooks();
        slimeCancels::registerHooks();
        magicSeries::registerHooks();

        //meleeFreeze::registerHooks();
        //reflectOnHit::registerHooks();
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}