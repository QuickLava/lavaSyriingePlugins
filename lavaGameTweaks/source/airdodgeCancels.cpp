#include "airdodgeCancels.h"
#include <so/so_external_value_accesser.h>

namespace airdodgeCancels
{
    const char outputTag[] = "[lavaAirdodgeCancels] ";
    const char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

    const u32 allTauntPadMask = 
        INPUT_PAD_BUTTON_MASK_APPEAL_HI | INPUT_PAD_BUTTON_MASK_APPEAL_S | INPUT_PAD_BUTTON_MASK_APPEAL_LW |
        INPUT_PAD_BUTTON_MASK_APPEAL_S_L | INPUT_PAD_BUTTON_MASK_APPEAL_S_R;    

    float currMeterArray[fighterHooks::maxFighterCount] = {};
    u8 currMeterStocksArray[fighterHooks::maxFighterCount] = {};
    u8 infiniteMeterModeFlags = 0;

    const u32 hitboxConnectedVar = 0x22000020;
    const u32 meterPaidVar = 0x22000021;
    const float maxMeter = 50.0f;
    const u32 maxStocks = 0x5;
    Vec3f zeroVec = { 0.0f, 0.0f, 0.0f };
    char graphicRootBoneName[] = "XRotN";
    const float indirectConnectMaxCancelDistance = 30.0f;
    const float onCancelSlowRadius = 30.0f;

    void doMeterReset(Fighter* fighterIn)
    {
        u8 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            currMeterArray[fighterPlayerNo] = 0.0f;
            currMeterStocksArray[fighterPlayerNo] = 0;
        }
    }
    void doMeterGain(Fighter* fighterIn, float damage)
    {
        u8 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;

            float currMeter = currMeterArray[fighterPlayerNo];
            u8 currStocks = currMeterStocksArray[fighterPlayerNo];

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

            currMeterArray[fighterPlayerNo] = currMeter;
            currMeterStocksArray[fighterPlayerNo] = currStocks;

            OSReport_N(meterChangeStr, outputTag, fighterPlayerNo, "Attack Landed", damage, currStocks, currMeter);

        }
    }
    void clearInfiniteMeterFlags()
    {
        infiniteMeterModeFlags = 0;
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
        u8 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleEnum->m_statusModule;
            soTransitionModule* transitionModule = statusModule->m_transitionModule;
            soControllerModule* controllerModule = moduleEnum->m_controllerModule;
            
            float currMeter = currMeterArray[fighterPlayerNo];
            u8 currStocks = currMeterStocksArray[fighterPlayerNo];
            bool infiniteMeterMode = infiniteMeterModeFlags & (1 << fighterPlayerNo);

            ipPadButton justPressed = controllerModule->getTrigger();
            if (workManageModule->isFlag(hitboxConnectedVar)
                && (currStocks > 0 || infiniteMeterMode)
                && (justPressed.m_mask & allTauntPadMask))
            {
                statusModule->changeStatusForce(Fighter::Status_Escape_Air, fighterIn->m_moduleAccesser);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Attack);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Special);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Cliff);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Jump_Aerial);

                currStocks -= 1;
                currMeterStocksArray[fighterPlayerNo] = currStocks;

                workManageModule->setFlag(1, meterPaidVar);
                moduleEnum->m_soundModule->playSE(snd_se_item_Ice_Crash, 1, 1, 0);
                moduleEnum->m_soundModule->playSE(snd_se_system_collection_delete, 1, 1, 0);

                u32 targetBoneID = moduleEnum->m_modelModule->getNodeId(graphicRootBoneName);
                moduleEnum->m_effectModule->reqFollow(ef_ptc_common_hit_ice, targetBoneID, &zeroVec, &zeroVec, 1.0f, 0, 0, 0, 0);
                u32 circleEfHandle = moduleEnum->m_effectModule->reqFollow(ef_ptc_common_guard_mark, targetBoneID, &zeroVec, &zeroVec, 3.0f, 0, 0, 0, 0);
                g_ecMgr->setSlowRate(circleEfHandle, 2);

                ftManager* fighterMgr = g_ftManager;
                const int fighterCount = fighterMgr->getEntryCount();
                Vec3f fighterPos = moduleEnum->m_postureModule->getPrevPos();
                for (int i = 0; i < fighterCount; i++)
                {
                    int currEntryID = fighterMgr->getEntryIdFromIndex(i);
                    Fighter* currFighter = fighterMgr->getFighter(currEntryID, 0);
                    if (soExternalValueAccesser::getTeamNo(fighterIn) == soExternalValueAccesser::getTeamNo(currFighter)) continue;

                    Vec3f currFighterPos = currFighter->m_moduleAccesser->getPostureModule()->getPrevPos();
                    float distance = fighterPos.distance(&currFighterPos);
                    if (distance < onCancelSlowRadius)
                    {
                        currFighter->setSlow(1, 2, 20, 1);
                    }
                }
                OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterPlayerNo(fighterIn), "Airdodge Cancel", -maxMeter, currStocks, currMeter);
            }

            ipPadButton pressed = controllerModule->getButton();
            if (pressed.m_attack && pressed.m_special && pressed.m_jump && (justPressed.m_mask & allTauntPadMask))
            {
                infiniteMeterModeFlags ^= (1 << fighterPlayerNo);
                doMeterReset(fighterIn);
                OSReport_N("%sInfinite Meter Mode Status: %d!\n", outputTag, !infiniteMeterMode);
            }
        }
    }

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerMeleeOnStartCallback(clearInfiniteMeterFlags);
        fighterHooks::ftCallbackMgr::registerOnAttackCallback(onHitCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackItemCallback((fighterHooks::FighterOnAttackItemCB)onIndirectHitCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackArticleCallback((fighterHooks::FighterOnAttackArticleCB)onIndirectHitCallback);
        fighterHooks::ftCallbackMgr::registerOnCreateCallback(doMeterReset);
        fighterHooks::ftCallbackMgr::registerOnUpdateCallback(onUpdateCallback);
    }
}