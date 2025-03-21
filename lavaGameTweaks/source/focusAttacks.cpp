#include "focusAttacks.h"

namespace focusAttacks
{
    char outputTag[] = "[focusAttacks] ";

    const u32 crumpled = 0x2200003B;
    const u32 receivedArmor = 0x2200003B;
    const float minChargeForFocus = 0.5f;

    u8 focusChargedFlags = 0x00;

    enum smashAttackState
    {
        sas_NONE = 0x00,
        sas_START,
        sas_CHARGE,
        sas_ATTACK
    };
    void enableDashCancel(soStatusModuleImpl* statusModuleIn)
    {
        soTransitionModule* transitionModule = statusModuleIn->m_transitionModule;
        transitionModule->enableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Ground);
        transitionModule->unableTermAll(Fighter::Status_Transition_Term_Group_Chk_Ground);
        transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Dash, Fighter::Status_Transition_Term_Group_Chk_Ground);
        transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Turn_Dash, Fighter::Status_Transition_Term_Group_Chk_Ground);
        transitionModule->unableTermGroup(Fighter::Status_Transition_Term_Group_Chk_Ground_Escape);
    }
    smashAttackState classifySmashAttackState(u32 statusIn)
    {
        smashAttackState result = sas_NONE;

        switch (statusIn)
        {
        case Fighter::Status_Attack_Hi4_Start: case Fighter::Status_Attack_S4_Start: case Fighter::Status_Attack_Lw4_Start: { result = sas_START; break; }
        case Fighter::Status_Attack_Hi4_Hold: case Fighter::Status_Attack_S4_Hold: case Fighter::Status_Attack_Lw4_Hold: { result = sas_CHARGE; break; }
        case Fighter::Status_Attack_Hi4: case Fighter::Status_Attack_S4: case Fighter::Status_Attack_Lw4: { result = sas_ATTACK; break; }
        }

        return result;
    }

    void onAttackCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile, u32 attackKind, u32 attackSituation)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_FOCUS_ATTACKS) && target->m_taskCategory == gfTask::Category_Fighter)
        {
            if (focusChargedFlags & (1 << fighterPlayerNo)
                && classifySmashAttackState(attacker->m_moduleAccesser->getStatusModule()->getStatusKind()) == sas_ATTACK)
            {
                u32 targetStatus;
                OSReport_N("%s", outputTag);
                soModuleEnumeration* moduleEnum = target->m_moduleAccesser->m_enumerationStart;
                soStatusModule* statusModule = moduleEnum->m_statusModule;
                if (moduleEnum->m_situationModule->getKind() == 0 && damage >= 5.0f)
                {
                    statusModule->changeStatus(Fighter::Status_Down_Spot, target->m_moduleAccesser);
                    moduleEnum->m_damageModule->sleep(1);
                    moduleEnum->m_damageModule->addDamage(damage, 0);
                    moduleEnum->m_collisionHitModule->setCheckCatch(1, 0);
                    moduleEnum->m_workManageModule->onFlag(crumpled);
                    moduleEnum->m_motionModule->setRate(0.5f);
                    moduleEnum->m_stopModule->setHitStopFrame(30, 0);
                    mechUtil::reqCenteredGraphic(target, ef_ptc_common_hit_normal_shock_wave, 1.2f, 0);
                    u32 dustGHXHandle = moduleEnum->m_effectModule->req(ef_ptc_common_vertical_smoke_b, mechUtil::sbid_TransN,
                        &mechUtil::zeroVec, &mechUtil::zeroVec, 1.0f, &mechUtil::zeroVec, &mechUtil::zeroVec, 0, 0);
                    g_ecMgr->setSlowRate(dustGHXHandle, 2);
                    OSReport_N("Grounded ");
                }
                enableDashCancel((soStatusModuleImpl*)attacker->m_moduleAccesser->getStatusModule());
            }
        }
    }
    void onHitCallback(Fighter* target, StageObject* attacker, float damage)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(target);
        if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_FOCUS_ATTACKS))
        {
            if (classifySmashAttackState(target->m_moduleAccesser->getStatusModule()->getStatusKind()) == sas_CHARGE)
            {
                focusChargedFlags |= 1 << fighterPlayerNo;
            }
        }
    }
    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
            soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleEnum->m_statusModule;

            u32 focusChargedFlagsTemp = focusChargedFlags;
            u32 playerBitMask = 1 << fighterPlayerNo;

            u32 currStatus = statusModule->getStatusKind();
            smashAttackState smashState = classifySmashAttackState(currStatus);
            if (smashState == sas_NONE)
            {
                if (currStatus == Fighter::Status_Down_Spot)
                {
                    if (workManageModule->isFlag(crumpled))
                    {
                        fighterIn->m_moduleAccesser->getDamageModule()->sleep(0);
                        workManageModule->offFlag(crumpled);
                    }
                }

                focusChargedFlagsTemp &= ~playerBitMask;
            }
            else if(smashState != sas_ATTACK && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_FOCUS_ATTACKS))
            {
                if (!moduleEnum->m_workManageModule->isFlag(receivedArmor))
                {
                    moduleEnum->m_damageModule->setNoReactionModeStatus(120.0f, -1.0f, 2);
                    workManageModule->onFlag(receivedArmor);
                }
                if (focusChargedFlagsTemp & playerBitMask)
                {
                    enableDashCancel(statusModule);
                }
                else if (smashState == sas_CHARGE && mechUtil::currAnimProgress(fighterIn) >= minChargeForFocus)
                {
                    mechUtil::reqCenteredGraphic(fighterIn, ef_ptc_common_dead_flash, 1.0f, 0);
                    mechUtil::playSE(fighterIn, snd_se_item_Raygun_empty);
                    focusChargedFlagsTemp |= playerBitMask;
                }
            }

            focusChargedFlags = (u8)focusChargedFlagsTemp;
        }
    }

#pragma c99 on
    fighterHooks::cbBundle callbacks =
    {
        .FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .FighterOnAttackCB = (fighterHooks::FighterOnAttackCB)onAttackCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}