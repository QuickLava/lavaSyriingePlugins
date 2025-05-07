#include "fighterHooks.h"
#include "magicSeries.h"
#include "meleeGameSetFreeze.h"
#include "reflectOnHit.h"
#include "airdodgeCancels.h"
#include "slimeCancels.h"
#include "rocketBurst.h"
#include "finalSmashMeter.h"
#include "focusAttacks.h"
//#include "transitionListener.h"
#include "_mechanicsHub.h"

// Note: 0x8070AA14 is SORA_MELEE base address
namespace lavaGameTweaks
{
    void Init(CoreApi* api)
    {
        fighterHooks::registerFighterHooks();
        mechHub::populate();
        mechHub::registerHooks();
        airdodgeCancels::registerHooks();
        slimeCancels::registerHooks();
        magicSeries::registerHooks();
        rocketBurst::registerHooks();
        finalSmashMeter::registerHooks();
        focusAttacks::registerHooks();
        //transitionListener::registerHooks();

        //meleeFreeze::registerHooks();
        //reflectOnHit::registerHooks();
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}