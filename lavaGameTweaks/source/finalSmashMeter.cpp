#include "finalSmashMeter.h"

namespace finalSmashMeter
{
    char outputTag[] = "[finalSmashMeter] ";
    char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

    fighterMeters::meterBundle fsMeters[fighterHooks::maxFighterCount];

    const u32 maxStocks = 0x1;
    const float meterStockSize = 100.0f;
    const fighterMeters::meterConfiguration meterConf = { meterStockSize * maxStocks, meterStockSize };

    void onFighterCreateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_FINAL_SMASH_METER))
        {
            fighterMeters::meterBundle* targetMeterBundle = fsMeters + fighterPlayerNo;
            targetMeterBundle->setMeterConfig(meterConf, 1);
            OSReport_N("%sFinal Smash Meter On for P%d!\n", outputTag, fighterPlayerNo);
        }
    }
    void onAttackCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile, u32 attackKind, u32 attackSituation)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_FINAL_SMASH_METER)
            && attacker->m_entryId != g_ftManager->m_finalEntryId)
        {
            fighterMeters::meterBundle* targetMeterBundle = fsMeters + fighterPlayerNo;
            if (targetMeterBundle->addMeter(damage) > 0)
            {
                mechUtil::reqCenteredGraphic(attacker, ef_ptc_common_smash_ball, 1.0f, 1);
            }
            OSReport_N(meterChangeStr, outputTag, fighterPlayerNo, "Attack Landed",
                damage, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
            if (targetMeterBundle->getMeterStocks() > 0x0 && g_ftManager->m_finalEntryId == -1)
            {
                g_ftManager->setFinal(attacker->m_entryId, 0);
                targetMeterBundle->resetMeter();
                OSReport_N("%sFinal Smash Granted to P%d!\n", outputTag, fighterPlayerNo);
            }
        }
    }
    bool disableSmashBallDrop(Fighter* fighterIn, u32 comparisonValue)
    {
        bool result = 0;

        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        result = mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_FINAL_SMASH_METER);

        return result;
    }
    asm void disableSmashBallDropHook()
    {
        nofralloc
        mflr r31;                     // Backup LR in a non-volatile register!
                                      // Call main function body!
        mr r3, r27;                   // Copy Fighter* into r3...
        mr r4, r26;                   // ... and the calc'd comparison value into r4,
        bl disableSmashBallDrop;      // ... and call!
        cmplwi r3, 0x01;              // Check if we returned 0x1...
        bne+ allowDrop;               // ... and if not, we're allowed to drop the Smash ball, so skip the following.
        lmw r3, R3_STACK_BAK_OFF(r1); // Otherwise though, skip the drop! Restore our backups from the Trampoline stackframe...
        lwz r1, 0x00(r1);             // ... then deallocate the stack frame.
        lis r12, 0x8084;              // \ 
        ori r12, r12, 0x16FC;         // | 
        mtctr r12;                    // | And finally, jump past the item creation section.
        bctr;                         // /
    allowDrop:
        mtlr r31;                     // Restore LR...
        blr;                          // ... and return!
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnCreateCB = (fighterHooks::FighterOnCreateCB)onFighterCreateCallback,
        .m_FighterOnAttackCB = (fighterHooks::FighterOnAttackCB)onAttackCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        // Disable Smash Ball Drop Hook @ 0x80841638: 0x5F0 bytes into symbol "dropItemCheck/[Fighter]/fighter.o"
        SyringeCompat::syInlineHookRel(0x136C24, disableSmashBallDropHook, Modules::SORA_MELEE);
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}