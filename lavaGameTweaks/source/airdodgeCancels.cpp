#include "airdodgeCancels.h"
#include <so/so_external_value_accesser.h>

namespace airdodgeCancels
{
    char outputTag[] = "[lavaAirdodgeCancels] ";
    char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

    u8 infiniteMeterModeFlags = 0;
    const u32 maxStocks = 0x5;
    const float meterStockSize = 50.0f;
    const fighterMeters::meterConfiguration meterConf = { meterStockSize * maxStocks, meterStockSize };

    const u32 hitboxConnectedVar = 0x22000038;
    const u32 meterPaidVar = 0x22000039;

    const float indirectConnectMaxCancelDistance = 30.0f;
    const float onCancelSlowRadius = 30.0f;

    void onFighterCreateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_AIRDODGE_CANCELS))
        {
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            targetMeterBundle->setMeterConfig(meterConf, 1);
        }
    }
    void onHitCallback(Fighter* attacker, StageObject* target, float damage)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_AIRDODGE_CANCELS))
        {
            attacker->m_moduleAccesser->getWorkManageModule()->setFlag(1, hitboxConnectedVar);

            mechHub::doMeterGain(attacker, damage, ef_ptc_common_hit_ice, 0.75f, mechHub::announcerOnStockGain);
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            OSReport_N(meterChangeStr, outputTag, fighterPlayerNo, "Attack Landed",
                damage, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
        }
    }
    void onIndirectHitCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_AIRDODGE_CANCELS))
        {
            soModuleEnumeration* attackerModuleEnum = attacker->m_moduleAccesser->m_enumerationStart;
            soModuleEnumeration* targetModuleEnum = target->m_moduleAccesser->m_enumerationStart;
            if (targetModuleEnum != NULL)
            {
                OSReport_N(outputTag);
                mechHub::doMeterGain(attacker, damage, ef_ptc_common_hit_ice, 0.75f, mechHub::announcerOnStockGain);

                float distance = mechHub::getDistanceBetween(attacker, target, 1);
                OSReport_N("%sProjectile Connected %.2f Units Away!\n", outputTag, distance);
                if (distance <= indirectConnectMaxCancelDistance)
                {
                    attackerModuleEnum->m_workManageModule->setFlag(1, hitboxConnectedVar);
                }
            }
        }
    }
    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_AIRDODGE_CANCELS))
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleEnum->m_statusModule;
            soTransitionModule* transitionModule = statusModule->m_transitionModule;
            soControllerModule* controllerModule = moduleEnum->m_controllerModule;
            
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            u32 currMeterStocks = targetMeterBundle->getMeterStocks();
            bool infiniteMeterMode = (infiniteMeterModeFlags >> fighterPlayerNo) & 0b1;

            ipPadButton justPressed = controllerModule->getTrigger();
            if (workManageModule->isFlag(hitboxConnectedVar)
                && (currMeterStocks > 0 || infiniteMeterMode)
                && (justPressed.m_mask & mechHub::allTauntPadMask))
            {
                statusModule->changeStatusForce(Fighter::Status_Escape_Air, fighterIn->m_moduleAccesser);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Attack);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Special);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Jump_Aerial);

                targetMeterBundle->addMeterStocks(-1);

                workManageModule->setFlag(1, meterPaidVar);
                mechHub::playSE(fighterIn, snd_se_item_Ice_Crash);
                mechHub::playSE(fighterIn, snd_se_system_collection_delete);
                mechHub::reqCenteredGraphic(fighterIn, ef_ptc_common_hit_ice, 1.0f, 1);
                u32 circleEfHandle = mechHub::reqCenteredGraphic(fighterIn, ef_ptc_common_guard_mark, 3.0f, 1);
                g_ecMgr->setSlowRate(circleEfHandle, 2);

                ftManager* fighterMgr = g_ftManager;
                const int fighterCount = fighterMgr->getEntryCount();
                for (int i = 0; i < fighterCount; i++)
                {
                    int currEntryID = fighterMgr->getEntryIdFromIndex(i);
                    Fighter* currFighter = fighterMgr->getFighter(currEntryID, 0);
                    if (soExternalValueAccesser::getTeamNo(fighterIn) == soExternalValueAccesser::getTeamNo(currFighter)) continue;

                    float distance = mechHub::getDistanceBetween(fighterIn, currFighter, 1);
                    if (distance <= onCancelSlowRadius)
                    {
                        currFighter->setSlow(1, 2, 20, 1);
                    }
                }

                OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterPlayerNo(fighterIn), "Airdodge Cancel", 
                    -meterStockSize, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
            }

            ipPadButton pressed = controllerModule->getButton();
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
        fighterHooks::ftCallbackMgr::registerOnAttackItemCallback((fighterHooks::FighterOnAttackItemCB)onIndirectHitCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackArticleCallback((fighterHooks::FighterOnAttackArticleCB)onIndirectHitCallback);
        fighterHooks::ftCallbackMgr::registerOnCreateCallback(onFighterCreateCallback);
        fighterHooks::ftCallbackMgr::registerOnUpdateCallback(onUpdateCallback);
    }
}