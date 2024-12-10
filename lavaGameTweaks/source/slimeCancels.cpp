#include "slimeCancels.h"
#include <so/so_external_value_accesser.h>

namespace slimeCancels
{
    const char outputTag[] = "[lavaSlimeCancels] ";
    const char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

    u8 infiniteMeterModeFlags = 0;
    const u32 maxStocks = 0x3;
    const float meterStockSize = 33.0f;
    const fighterMeters::meterConfiguration meterConf = { meterStockSize * maxStocks, meterStockSize };

    const u32 meterPaidVar = 0x22000039;
    const u32 beenFrozenVar = 0x2200003A;
    const u32 didSlimeCancelVar = 0x2200003B;

    const u32 onCancelStopBaseDuration = 15;
    const float onCancelStopRadius = 20.0f;
    const float onCancelStopWindowLength = 5.0f;
    Vec3f onCancelStopKBMult = { 0.75f, 0.75f, 0.75f };
    const float onCancelSpeedLimit = 4.0f;

    bool getFlagForPlayer(u8 flagByte, u32 playerNo)
    {
        return flagByte & (1 << playerNo);
    }
    void setFlagForPlayer(u8& flagByte, u32 playerNo, bool stateIn)
    {
        flagByte &= ~(1 << playerNo);
        flagByte |= stateIn << playerNo;
    }
    void doMeterGain(Fighter* fighterIn, float damage)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;

            int initialStockCount = targetMeterBundle->getMeterStocks();
            targetMeterBundle->addMeter(damage);
            int finalStockCount = targetMeterBundle->getMeterStocks();
            int changeInStockCount = finalStockCount - initialStockCount;

            if (changeInStockCount > 0)
            {
                moduleEnum->m_effectModule->reqFollow(ef_ptc_common_cliff_catch,
                    mechHub::gfxRootBoneID, &mechHub::zeroVec, &mechHub::zeroVec, 2.5f, 0, 0, 0, 0);
            }

            OSReport_N(meterChangeStr, outputTag, fighterPlayerNo, "Attack Landed", damage, finalStockCount, targetMeterBundle->getMeterStockRemainder());
        }
    }
    
    void onFighterCreateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_SLIME_CANCELS))
        {
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            targetMeterBundle->setMeterConfig(meterConf, 1);
        }
    }
    void onHitCallback(Fighter* attacker, StageObject* target, float damage)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_SLIME_CANCELS))
        {
            doMeterGain(attacker, damage);
        }
    }
    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_SLIME_CANCELS))
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            soStatusModule* statusModule = moduleEnum->m_statusModule;
            soSituationModule* situationModule = moduleEnum->m_situationModule;
            soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
            soControllerModule* controllerModule = moduleEnum->m_controllerModule;
            
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            u32 currMeterStocks = targetMeterBundle->getMeterStocks();
            bool infiniteMeterMode = (infiniteMeterModeFlags >> fighterPlayerNo) & 0b1;

            ipPadButton justPressed = controllerModule->getTrigger();
            ipPadButton pressed = controllerModule->getButton();

            bool slimeCancelInput = 0;
            u32 currStatus = statusModule->getStatusKind();
            if ((currStatus >= Fighter::Status_Attack && currStatus <= Fighter::Status_Attack_Air)
                || (currStatus >= 0x112 && currStatus != 0x116))
            {
                slimeCancelInput = justPressed.m_mask & mechHub::allTauntPadMask;
            }
            
            if (workManageModule->isFlag(didSlimeCancelVar))
            {
                statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Tread_Jump);
                workManageModule->offFlag(didSlimeCancelVar);
                OSReport_N("%sRe-Enabled: Footstool!\n", outputTag);
            }

            if (slimeCancelInput && (currMeterStocks > 0 || infiniteMeterMode) && !workManageModule->isFlag(meterPaidVar))
            {
                targetMeterBundle->addMeterStocks(-1);
                workManageModule->setFlag(1, meterPaidVar);

                moduleEnum->m_soundModule->playSE(snd_se_item_pasaran_growth, 1, 1, 0);
                moduleEnum->m_soundModule->playSE(snd_se_item_spring_02, 1, 1, 0);
                u32 effectHandle = moduleEnum->m_effectModule->reqFollow(ef_ptc_common_ray_gun_shot,
                    mechHub::gfxRootBoneID, &mechHub::zeroVec, &mechHub::gfxFaceScreenRotVec, 1.0f, 0, 0, 0, 0);
                g_ecMgr->setScl(effectHandle, &mechHub::gfxFlattenSclVec);
                g_ecMgr->setSlowRate(effectHandle, 2);

                ftManager* fighterMgr = g_ftManager;
                const int fighterCount = fighterMgr->getEntryCount();
                Vec3f fighterPos = moduleEnum->m_postureModule->getPrevPos();
                for (int i = 0; i < fighterCount; i++)
                {
                    int currEntryID = fighterMgr->getEntryIdFromIndex(i);
                    Fighter* currFighter = fighterMgr->getFighter(currEntryID, 0);
                    soWorkManageModule* targetWorkManageModule = currFighter->m_moduleAccesser->getWorkManageModule();
                    if (targetWorkManageModule->isFlag(beenFrozenVar)) continue;

                    u32 currFtStatus = currFighter->m_moduleAccesser->getStatusModule()->getStatusKind();
                    if (currFtStatus < Fighter::Status_Damage || currFtStatus > Fighter::Status_Damage_Fly_Roll) continue;

                    float currAnimFrame = currFighter->m_moduleAccesser->getMotionModule()->getFrame();
                    if (currAnimFrame > onCancelStopWindowLength) continue;

                    soDamageLog* damageLog = currFighter->m_moduleAccesser->getDamageModule()->getDamageLog();
                    if (damageLog->m_attackerTeamOwnerId != fighterIn->m_taskId) continue;

                    soStopModule* attackerStopModule = fighterIn->m_moduleAccesser->m_enumerationStart->m_stopModule;
                    soStopModule* targetStopModule = currFighter->m_moduleAccesser->m_moduleEnumeration.m_stopModule;
                    u32 attackerHitstop = onCancelStopBaseDuration;
                    u32 targetHitstop = onCancelStopBaseDuration;

                    if (targetStopModule->isDamage())
                    {
                        targetHitstop += damageLog->m_hitStopFrame;
                        moduleEnum->m_soundModule->playSE(snd_se_Audience_Kansei_s, 1, 1, 0);
                        u32 remainingHitstop = targetStopModule->getHitStopRealFrame();
                        OSReport_N("%sPerfect Cancel: Defender Hitstop Restarted (+%d Frames)\n", outputTag, damageLog->m_hitStopFrame);
                    }

                    attackerStopModule->setHitStopFrame(attackerHitstop, 0);
                    targetStopModule->setHitStopFrame(targetHitstop, 1);
                    targetWorkManageModule->onFlag(beenFrozenVar);

                    soKineticEnergy* kbEnergy = currFighter->m_moduleAccesser->getKineticModule()->getEnergy(Fighter::Kinetic_Energy_Damage);
                    kbEnergy->mulSpeed(&onCancelStopKBMult);
                }

                soControllerImpl* controllerPtr = (soControllerImpl*)controllerModule->getController();
                controllerPtr->m_trigger &= ~mechHub::allTauntPadMask;
                if (moduleEnum->m_situationModule->getKind() == 0x00)
                {
                    statusModule->changeStatusForce(Fighter::Status_Wait, fighterIn->m_moduleAccesser);
                }
                else
                {
                    u32 tauntInputBak = controllerPtr->m_button & mechHub::allTauntPadMask;
                    controllerPtr->m_button &= ~tauntInputBak;
                    statusModule->changeStatusForce(Fighter::Status_Fall_Aerial, fighterIn->m_moduleAccesser);
                    statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Tread_Jump);
                    controllerPtr->m_button |= tauntInputBak;
                    workManageModule->onFlag(didSlimeCancelVar);
                }

                OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterPlayerNo(fighterIn), "Slime Cancel", 
                    -meterStockSize, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
            }

            if (pressed.m_attack && pressed.m_special && pressed.m_jump && (justPressed.m_mask & mechHub::allTauntPadMask))
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

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerMeleeOnStartCallback(onMeleeStartCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackCallback(onHitCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackItemCallback((fighterHooks::FighterOnAttackItemCB)onHitCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackArticleCallback((fighterHooks::FighterOnAttackArticleCB)onHitCallback);
        fighterHooks::ftCallbackMgr::registerOnCreateCallback(onFighterCreateCallback);
        fighterHooks::ftCallbackMgr::registerOnUpdateCallback(onUpdateCallback);
    }
}