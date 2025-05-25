#include "fighterHooks.h"
#include "magicSeries.h"
#include "meleeGameSetFreeze.h"
#include "reflectOnHit.h"
#include "airdodgeCancels.h"
#include "slimeCancels.h"
#include "rocketBurst.h"
#include "finalSmashMeter.h"
#include "focusAttacks.h"
#include "squatDodge.h"
#include "horizontalWavedashes.h"
#include "actionableAirdodge.h"
//#include "transitionListener.h"
#include "_mechanicsHub.h"

// Note: 0x8070AA14 is SORA_MELEE base address, Ghidra is 0x807189A0, 0xDF8C Difference
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
        rocketBurst::registerHooks();
        finalSmashMeter::registerHooks();
        focusAttacks::registerHooks();
        horiWavedashes::registerHooks();
        squatDodge::registerHooks();
        actionableAirdodge::registerHooks();    
        //transitionListener::registerHooks();

        //meleeFreeze::registerHooks();
        //reflectOnHit::registerHooks();
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}