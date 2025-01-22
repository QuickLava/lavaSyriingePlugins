#include "focusAttacks.h"

namespace focusAttacks
{
    char outputTag[] = "[focusAttacks] ";

    const u32 crumpled = 0x2200003B;
    const u32 inflictedCrumple = 0x2200003C;

    void onAttackCallback(Fighter* attacker, StageObject* target, float damage)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_FOCUS_ATTACKS)
            && target->m_taskCategory == gfTask::Category_Fighter)
        {
            u32 attackerStatus = attacker->m_moduleAccesser->getStatusModule()->getStatusKind();
            if (attackerStatus == Fighter::Status_Attack_Hi4 || attackerStatus == Fighter::Status_Attack_S4 || attackerStatus == Fighter::Status_Attack_Lw4)
            {
                float chargeProgress = attacker->m_moduleAccesser->getWorkManageModule()->getFloat(Fighter::Status_Work_Float_Reserve_Hold_Rate);
                OSReport_N("%sFighter 0%d: Charge Progress %.2f\n", outputTag, fighterPlayerNo, chargeProgress);
                if (chargeProgress > 0.75f)
                {
                    u32 targetStatus;
                    OSReport_N("%s", outputTag);
                    soModuleEnumeration* moduleEnum = target->m_moduleAccesser->m_enumerationStart;
                    if (moduleEnum->m_situationModule->getKind() == 0)
                    {
                        if (moduleEnum->m_statusModule->getStatusKind() != Fighter::Status_Down_Spot)
                        {
                            moduleEnum->m_statusModule->changeStatus(Fighter::Status_Down_Spot, target->m_moduleAccesser);
                            moduleEnum->m_damageModule->sleep(1);
                            moduleEnum->m_damageModule->addDamage(damage, 0);
                            moduleEnum->m_workManageModule->onFlag(crumpled);
                            moduleEnum->m_motionModule->setRate(0.5f);
                            moduleEnum->m_stopModule->setHitStopFrame(30, 0);
                            u32 dustGHXHandle = moduleEnum->m_effectModule->req(ef_ptc_common_vertical_smoke_b, mechUtil::sbid_TransN,
                                &mechUtil::zeroVec, &mechUtil::zeroVec, 1.0f, &mechUtil::zeroVec, &mechUtil::zeroVec, 0, 0);
                            g_ecMgr->setSlowRate(dustGHXHandle, 2);
                            OSReport_N("Grounded ");
                        }
                    }
                    else
                    {
                        moduleEnum->m_statusModule->changeStatus(Fighter::Status_Shield_Break_Fly, target->m_moduleAccesser);
                        OSReport_N("Aerial ");
                    }
                }
            }
        }
    }
    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            u32 currStatus = moduleEnum->m_statusModule->getStatusKind();
            if (currStatus == Fighter::Status_Down_Spot)
            {
                if (moduleEnum->m_workManageModule->isFlag(crumpled))
                {
                    fighterIn->m_moduleAccesser->getDamageModule()->sleep(0);
                    moduleEnum->m_workManageModule->offFlag(crumpled);
                }
            }
            else if (currStatus)
            {

            }
        }
    }
    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerOnAttackCallback(onAttackCallback);
        fighterHooks::ftCallbackMgr::registerOnAttackArticleCallback((fighterHooks::FighterOnAttackArticleCB)onAttackCallback);
        fighterHooks::ftCallbackMgr::registerOnUpdateCallback(onUpdateCallback);
    }
}