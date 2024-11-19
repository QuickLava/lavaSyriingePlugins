#include "airdodgeCancels.h"
#include <ft/fighter.h>

namespace airdodgeCancels
{
    const char outputTag[] = "[lavaAirdodgeCancels] ";
    const char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

    const u32 currMeterVar = 0x11000060;
    const u32 currMeterStocksVar = 0x1000004A;
    const u32 infiniteMeterModeVar = 0x12000070;
    const u32 hitboxConnectedVar = 0x22000020;
    const u32 meterPaidVar = 0x22000021;
    const float maxMeter = 50.0f;
    const u32 maxStocks = 0x5;
    Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
    char graphicRootBoneName[] = "XRotN";
    const float indirectConnectMaxCancelDistance = 30.0f;

    void doMeterReset(Fighter* fighterIn)
    {
        soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
        if (moduleEnum != NULL)
        {
            soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
            workManageModule->setFloat(0.0f, currMeterVar);
            workManageModule->setInt(0x00, currMeterStocksVar);
        }
    }
    void doMeterGain(Fighter* fighterIn, float damage)
    {
        soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
        if (moduleEnum != NULL)
        {
            u8 slotNo = fighterHooks::getFighterSlotNo(fighterIn);
            soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;

            float currMeter = workManageModule->getFloat(currMeterVar);
            u32 currStocks = workManageModule->getInt(currMeterStocksVar);

            currMeter += damage;
            if (currMeter >= maxMeter && currStocks < maxStocks)
            {
                int gainedStocks = currMeter / maxMeter;
                currMeter -= gainedStocks * maxMeter;
                currStocks += (int)gainedStocks;
                currStocks = MIN(currStocks, maxStocks);

                moduleEnum->m_soundModule->playSE((SndID)((snd_se_narration_one + 1) - currStocks), 1, 1, 0);
                u32 targetBoneID = moduleEnum->m_modelModule->getNodeId(graphicRootBoneName);
                moduleEnum->m_effectModule->reqFollow(ef_ptc_common_hit_ice, targetBoneID, &zeroVec, &zeroVec, 0.75f, 0, 0, 0, 0);
            }
            currMeter = MIN(currMeter, maxMeter);

            workManageModule->setFloat(currMeter, currMeterVar);
            workManageModule->setInt(currStocks, currMeterStocksVar);

            OSReport_N(meterChangeStr, outputTag, slotNo, "Attack Landed", damage, currStocks, currMeter);
        }
    }

    void onHitCallback(Fighter* attacker, StageObject* target, float damage)
    {
        soModuleEnumeration* moduleEnum = attacker->m_moduleAccesser->m_enumerationStart;
        if (moduleEnum != NULL)
        {
            doMeterGain(attacker, damage);
            moduleEnum->m_workManageModule->setFlag(1, hitboxConnectedVar);
        }
    }
    void onIndirectHitCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile)
    {
        soModuleEnumeration* attackerModuleEnum = attacker->m_moduleAccesser->m_enumerationStart;
        soModuleEnumeration* targetModuleEnum = target->m_moduleAccesser->m_enumerationStart;

        if (attackerModuleEnum != NULL && targetModuleEnum != NULL)
        {
            doMeterGain(attacker, damage);

            Vec3f attackerPos = attackerModuleEnum->m_postureModule->getPrevPos();
            Vec3f targetPos = targetModuleEnum->m_postureModule->getPrevPos();
            float distance = attackerPos.distance(&targetPos);
            OSReport_N("%sProjectile Connected %.2f Units Away!\n", outputTag, distance);
            if (distance < indirectConnectMaxCancelDistance)
            {
                attackerModuleEnum->m_workManageModule->setFlag(1, hitboxConnectedVar);
            }
        }
    }
    void onUpdateCallback(Fighter* fighterIn)
    {
        soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
        if (moduleEnum != NULL)
        {
            soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleEnum->m_statusModule;
            soTransitionModule* transitionModule = statusModule->m_transitionModule;
            soControllerModule* controllerModule = moduleEnum->m_controllerModule;
            
            ipButton justPressed = controllerModule->getTrigger();
            if (workManageModule->isFlag(hitboxConnectedVar)
                && (workManageModule->getInt(currMeterStocksVar) > 0 || workManageModule->isFlag(infiniteMeterModeVar))
                && justPressed.m_bits & (ipButton::_APPEAL_HI | ipButton::_APPEAL_LW | ipButton::_APPEAL_S | ipButton::_APPEAL_SL | ipButton::_APPEAL_SR))
            {
                statusModule->changeStatusForce(Fighter::Status_Escape_Air, fighterIn->m_moduleAccesser);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Attack);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Special);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Cliff);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Jump_Aerial);

                workManageModule->addInt(-1, currMeterStocksVar);
                workManageModule->setFlag(1, meterPaidVar);
                moduleEnum->m_soundModule->playSE(snd_se_item_Ice_Crash, 1, 1, 0);

                u32 targetBoneID = moduleEnum->m_modelModule->getNodeId(graphicRootBoneName);
                moduleEnum->m_effectModule->reqFollow(ef_ptc_common_hit_ice, targetBoneID, &zeroVec, &zeroVec, 1.0f, 0, 0, 0, 0);
                moduleEnum->m_effectModule->reqFollow(ef_ptc_common_cliff_catch, targetBoneID, &zeroVec, &zeroVec, 3.0f, 0, 0, 0, 0);

                OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterSlotNo(fighterIn), "Airdodge Cancel", -maxMeter,
                    workManageModule->getInt(currMeterStocksVar), workManageModule->getFloat(currMeterVar));
            }

            ipButton pressed = controllerModule->getButton();
            if (pressed.m_attack && pressed.m_special && pressed.m_jump
                && justPressed.m_bits & (ipButton::_APPEAL_HI | ipButton::_APPEAL_LW | ipButton::_APPEAL_S | ipButton::_APPEAL_SL | ipButton::_APPEAL_SR))
            {
                bool infiniteMode = !workManageModule->isFlag(infiniteMeterModeVar);
                workManageModule->setFlag(infiniteMode, infiniteMeterModeVar);
                doMeterReset(fighterIn);
                OSReport_N("%sInfinite Meter Mode Status: %d!\n", outputTag, infiniteMode);
            }
        }
    }

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerOnAttackCallback(onHitCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackItemCallback((fighterHooks::FighterOnAttackItemCB)onIndirectHitCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackArticleCallback((fighterHooks::FighterOnAttackArticleCB)onIndirectHitCallback);
        fighterHooks::ftCallbackMgr::registerOnCreateCallback(doMeterReset);
        fighterHooks::ftCallbackMgr::registerOnUpdateCallback(onUpdateCallback);
    }
}