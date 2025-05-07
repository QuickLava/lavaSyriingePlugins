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
        fighterHooks::registerFighterHooks(api);
        mechHub::populate();
        mechHub::registerHooks(api);
        airdodgeCancels::registerHooks(api);
        slimeCancels::registerHooks(api);
        magicSeries::registerHooks(api);
        rocketBurst::registerHooks(api);
        finalSmashMeter::registerHooks(api);
        focusAttacks::registerHooks(api);
        //transitionListener::registerHooks(api);

        //meleeFreeze::registerHooks(api);
        //reflectOnHit::registerHooks(api);
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}