#include "slimeCancels.h"
#include <so/so_external_value_accesser.h>

namespace slimeCancels
{
    char outputTag[] = "[slimeCancels] ";
    char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

    u8 infiniteMeterModeFlags = 0;
    const u32 maxStocks = 0x3;
    const float meterStockSize = 33.0f;
    const fighterMeters::meterConfiguration meterConf = { meterStockSize * maxStocks, meterStockSize };

    const u32 meterPaidVar = 0x22000039;
    const u32 beenFrozenVar = 0x2200003A;

    const u32 onCancelStopBaseDuration = 20;
    const float onCancelStopWindowLength = 5.0f;
    Vec3f onCancelStopKBMult = Vec3f(0.666f, 0.666f, 0.666f);

    void onFighterCreateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_SLIME_CANCELS))
        {
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            targetMeterBundle->setMeterConfig(meterConf, 1);
            OSReport_N("%sMeter Reset Handled\n", outputTag);
        }
    }
    void onAttackCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile, u32 attackKind, u32 attackSituation)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_SLIME_CANCELS))
        {
            mechUtil::doMeterGain(attacker, damage, ef_ptc_common_cliff_catch, 2.5f, mechUtil::mgac_ON_STOCK_GAIN);
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            OSReport_N(meterChangeStr, outputTag, fighterPlayerNo, "Attack Landed",
                damage, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
        }
    }
    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_SLIME_CANCELS))
        {
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            if (mechHub::getActiveMechanicEnabledDiff(fighterPlayerNo, mechHub::amid_AIRDODGE_CANCELS))
            {
                targetMeterBundle->setMeterConfig(meterConf, 1);
            }

            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            soStatusModule* statusModule = moduleEnum->m_statusModule;
            soSituationModule* situationModule = moduleEnum->m_situationModule;
            soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
            soControllerModule* controllerModule = moduleEnum->m_controllerModule;
            
            u32 currMeterStocks = targetMeterBundle->getMeterStocks();
            bool infiniteMeterMode = (infiniteMeterModeFlags >> fighterPlayerNo) & 0b1;

            ipPadButton justPressed = controllerModule->getTrigger();
            ipPadButton pressed = controllerModule->getButton();

            bool slimeCancelInput = 0;
            u32 currStatus = statusModule->getStatusKind();
            if (mechUtil::isAttackingStatusKind(currStatus))
            {
                slimeCancelInput = justPressed.m_mask & mechUtil::allTauntPadMask;
            }
            
            if (slimeCancelInput && (currMeterStocks > 0 || infiniteMeterMode) && !workManageModule->isFlag(meterPaidVar))
            {
                targetMeterBundle->addMeterStocks(-1);
                workManageModule->setFlag(1, meterPaidVar);

                mechUtil::playSE(fighterIn, snd_se_item_spring_02);
                mechUtil::playSE(fighterIn, snd_se_item_pasaran_growth);
                mechUtil::reqCenteredGraphic(fighterIn, ef_ptc_pokemon_latiaslatios_03, 0.75f, 1);
                u32 effectHandle = mechUtil::reqCenteredGraphic(fighterIn, ef_ptc_common_ray_gun_shot, 1.0f, 1);
                g_ecMgr->setRot(effectHandle, &mechUtil::gfxFaceScreenRotVec);
                g_ecMgr->setScl(effectHandle, &mechUtil::gfxFlattenSclVec);
                g_ecMgr->setSlowRate(effectHandle, 2);

                ftManager* fighterMgr = g_ftManager;
                const int fighterCount = fighterMgr->getEntryCount();
                for (int i = 0; i < fighterCount; i++)
                {
                    int currEntryID = fighterMgr->getEntryIdFromIndex(i);
                    Fighter* currFighter = fighterMgr->getFighter(currEntryID, 0);
                    soWorkManageModule* targetWorkManageModule = currFighter->m_moduleAccesser->m_enumerationStart->m_workManageModule;
                    if (targetWorkManageModule->isFlag(beenFrozenVar)) continue;

                    u32 currFtStatus = currFighter->m_moduleAccesser->getStatusModule().getStatusKind();
                    if (!mechUtil::isDamageStatusKind(currFtStatus)) continue;

                    float currAnimFrame = currFighter->m_moduleAccesser->getMotionModule().getFrame();
                    if (currAnimFrame > onCancelStopWindowLength) continue;

                    soDamageLog* damageLog = currFighter->m_moduleAccesser->getDamageModule().getDamageLog();
                    if (damageLog->m_attackerTeamOwnerId != fighterIn->m_taskId) continue;

                    soStopModule* attackerStopModule = fighterIn->m_moduleAccesser->m_enumerationStart->m_stopModule;
                    soStopModule* targetStopModule = currFighter->m_moduleAccesser->m_moduleEnumeration.m_stopModule;

                    u32 attackerHitstop = onCancelStopBaseDuration;
                    u32 targetHitstop = onCancelStopBaseDuration;
                    if (targetStopModule->isDamage())
                    {
                        u32 hitstopBonus = damageLog->m_hitStopFrame / 2;
                        targetHitstop += hitstopBonus;
                        OSReport_N("%sPerfect Cancel: Defender Hitstop Restarted (+%d Frames)\n", outputTag, hitstopBonus);
                    }

                    attackerStopModule->setHitStopFrame(attackerHitstop, 0);
                    targetStopModule->setHitStopFrame(targetHitstop, 0);
                    targetWorkManageModule->onFlag(beenFrozenVar);

                    soKineticEnergy* kbEnergy = currFighter->m_moduleAccesser->getKineticModule().getEnergy(Fighter::Kinetic_Energy_Id_Damage);
                    kbEnergy->mulSpeed(&onCancelStopKBMult);
                }

                soControllerImpl* controllerPtr = (soControllerImpl*)controllerModule->getController();
                controllerPtr->m_trigger &= ~mechUtil::allTauntPadMask;
                if (moduleEnum->m_situationModule->getKind() == 0x00)
                {
                    statusModule->changeStatusForce(Fighter::Status::Wait, fighterIn->m_moduleAccesser);
                }
                else
                {
                    workManageModule->setInt(0x1, Fighter::Instance_Work_Int_No_Tread_Frame);
                    statusModule->changeStatusForce(Fighter::Status::Fall_Aerial, fighterIn->m_moduleAccesser);
                }

                OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterPlayerNo(fighterIn), "Slime Cancel", 
                    -meterStockSize, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
            }

            if (pressed.m_attack && pressed.m_special && pressed.m_jump && (justPressed.m_mask & mechUtil::allTauntPadMask))
            {
                infiniteMeterModeFlags ^= (1 << fighterPlayerNo);
                targetMeterBundle->resetMeter();
                OSReport_N("%sInfinite Meter Mode Status: %d!\n", outputTag, !infiniteMeterMode);
            }
        }
    }

    void onMeleeStartCallback()
    {
        infiniteMeterModeFlags = 0;
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_MeleeOnStartCB = (fighterHooks::MeleeOnStartCB)onMeleeStartCallback,
        .m_FighterOnCreateCB = (fighterHooks::FighterOnCreateCB)onFighterCreateCallback,
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_FighterOnAttackCB = (fighterHooks::FighterOnAttackCB)onAttackCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}