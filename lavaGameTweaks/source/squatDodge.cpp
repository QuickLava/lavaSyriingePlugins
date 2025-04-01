#include <ft/fighter.h>
#include <st/st_utility.h>
#include "squatDodge.h"

using namespace codeMenu;
namespace squatDodge
{
    char outputTag[] = "[rivalsMode] ";

    const float attachDistance = 5.0f;
    Vec3f searchVector = { 0.0f, -attachDistance, 0.0f };

    const u32 parryActiveBit = 0x2200001F;
    const u32 airdodgeTimerVar = 0x20000001;
    const float setShieldSize = 60.0f;

    u8 dodgeSpent;
    u8 dodgeBuffered;
    u8 walljumpSpent;
    u8 parryBuffered;

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            u32 currStatus = statusModule->getStatusKind();

            u32 playerBit = 1 << fighterPlayerNo;
            u32 dodgeSpentTemp = dodgeSpent;
            u32 dodgeBufferedTemp = dodgeBuffered;
            u32 walljumpSpentTemp = walljumpSpent;
            u32 parryBufferedTemp = parryBuffered;

            if (currStatus == Fighter::Status_Jump_Squat)
            {
                if (moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger().m_guard)
                {
                    dodgeBufferedTemp |= playerBit;
                }
                if (mechUtil::currAnimProgress(fighterIn) == 1.0f && dodgeBufferedTemp & playerBit)
                {
                    statusModule->changeStatusRequest(Fighter::Status_Escape_Air, moduleAccesser);
                    dodgeBufferedTemp &= ~playerBit;
                }
            }

            if (currStatus == Fighter::Status_Escape_Air)
            {
                float currAnimProgress = mechUtil::currAnimProgress(fighterIn);
                int airdodgeTimer = workManageModule->getInt(airdodgeTimerVar);

                if (currAnimProgress <= 0.25)
                {
                    soGroundModule* groundModule = moduleAccesser->m_enumerationStart->m_groundModule;
                    Vec3f currPos;
                    Vec2f* currPosCastAddr = (Vec2f*)(&currPos);
                    *currPosCastAddr = groundModule->getDownPos(0);
                    currPos.m_z = 0.0f;

                    int lineIDOut;
                    Vec3f hitPosOut;
                    Vec3f normalVecOut;
                    if (stRayCheck(&currPos, &searchVector, &lineIDOut, &hitPosOut, &normalVecOut, 1, 0, 1))
                    {
                        float distanceFromGround = currPos.m_y - hitPosOut.m_y;
                        OSReport_N("%sDistanceFromGround: %.3f\n", outputTag, distanceFromGround);
                        soPostureModule* postureModule = moduleAccesser->m_enumerationStart->m_postureModule;
                        groundModule->setCorrect(soGroundShapeImpl::Correct_None, 0);
                        groundModule->setShapeSafePos((Vec2f*)&hitPosOut, 0);
                        postureModule->setPos(&hitPosOut);
                        groundModule->setCorrect(soGroundShapeImpl::Correct_Air, 0);
                        groundModule->attachGround(0);
                        groundModule->apply();
                    }
                }
                if (airdodgeTimer <= 0x00)
                {
                    dodgeSpentTemp |= playerBit;
                    airdodgeTimer--;
                }
                workManageModule->setInt(airdodgeTimer, airdodgeTimerVar);
                if (airdodgeTimer <= -0x06)
                {
                    workManageModule->setInt(0, airdodgeTimerVar);
                    statusModule->changeStatus(Fighter::Status_Fall_Aerial, moduleAccesser);
                }
            }
            else if (currStatus == Fighter::Status_Attack)
            {
                soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                u32 currMotion = motionModule->getKind();
                if (mechUtil::currAnimProgress(fighterIn) >= 0.25 
                    && currMotion == Fighter::Motion_Attack_11 || currMotion == Fighter::Motion_Attack_12)
                {
                    soTransitionModule* transitionModule = statusModule->m_transitionModule;
                    transitionModule->enableTermGroup(0x4);
                    transitionModule->unableTermAll(0x4);
                    transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_S3, Fighter::Status_Transition_Term_Group_Chk_Ground_Attack);
                    transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_Hi3, Fighter::Status_Transition_Term_Group_Chk_Ground_Attack);
                    transitionModule->enableTerm(Fighter::Status_Transition_Term_Cont_Attack_Lw3, Fighter::Status_Transition_Term_Group_Chk_Ground_Attack);
                }
            }
            else if (currStatus == Fighter::Status_FuraFura)
            {
                statusModule->changeStatus(Fighter::Status_FuraFura_End, moduleAccesser);
            }
            else if (currStatus == Fighter::Status_Wall_Jump)
            {
                walljumpSpentTemp |= playerBit;
            }
            else if (currStatus == Fighter::Status_Guard_On || currStatus == Fighter::Status_Guard)
            {
                soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
                if (controllerModule->getTrigger().m_special)
                {
                    soTransitionModule* transitionModule = statusModule->m_transitionModule;
                    statusModule->changeStatusRequest(Fighter::Status_Guard_Off, moduleAccesser);
                    parryBufferedTemp |= playerBit;
                }
            }
            else if (currStatus == Fighter::Status_Guard_Off)
            {
                soDamageModule* damageModule = moduleAccesser->m_enumerationStart->m_damageModule;
                if (parryBufferedTemp & playerBit)
                {
                    moduleAccesser->m_enumerationStart->m_effectModule->req(ef_ptc_common_vertical_smoke_b, mechUtil::sbid_TransN,
                        &mechUtil::zeroVec, &mechUtil::zeroVec, 1.0f, &mechUtil::zeroVec, &mechUtil::zeroVec, 0, 0);
                    fighterIn->setSlow(1, 2, 30, 0);
                    parryBufferedTemp &= ~playerBit;
                    damageModule->setNoReactionModeStatus(500.0f, -1.0f, 2);
                    damageModule->setDamageMul(0.0f);
                    workManageModule->onFlag(parryActiveBit);
                }
                if (workManageModule->isFlag(parryActiveBit) && mechUtil::currAnimProgress(fighterIn) >= 0.5f)
                {
                    damageModule->resetNoReactionModeStatus();
                    damageModule->setDamageMul(1.0f);
                    workManageModule->offFlag(parryActiveBit);
                }
            }

            u32 currSituation = moduleAccesser->m_enumerationStart->m_situationModule->getKind();
            if (currSituation == Situation_Ground || currSituation == Situation_Cliff)
            {
                dodgeSpentTemp &= ~playerBit;
                walljumpSpentTemp &= ~playerBit;
            }
            else if (currSituation == Situation_Air)
            {
                if (mechUtil::isDamageStatusKind(currStatus))
                {
                    dodgeSpentTemp &= ~playerBit;
                    walljumpSpentTemp &= ~playerBit;
                }
                else
                {
                    if (dodgeSpentTemp & playerBit)
                    {
                        statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Escape);
                    }
                    if (walljumpSpentTemp & playerBit)
                    {
                        statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                    }
                }
            }

            if (currStatus == Fighter::Status_Dash && moduleAccesser->m_enumerationStart->m_motionModule->getFrame() < 3.0f)
            {
                float stickAbs = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Variable_Float_Controller_Stick_X_Abs, 0);
                if (stickAbs < 0.2f)
                {
                    soKineticModule* kineticModule = moduleAccesser->m_enumerationStart->m_kineticModule;
                    soInstanceAttribute energyMask = { 0xFFFF };
                    Vec3f currSpeed = kineticModule->getSumSpeed3f(&energyMask);
                    currSpeed.m_x *= moduleAccesser->m_enumerationStart->m_postureModule->getLr() * 0.75f;
                    statusModule->changeStatus(Fighter::Status_Wait, moduleAccesser);
                    kineticModule->addSpeed(&currSpeed, moduleAccesser);
                }
            }

            dodgeSpent = dodgeSpentTemp;
            dodgeBuffered = dodgeBufferedTemp;
            walljumpSpent = walljumpSpentTemp;
            parryBuffered = parryBufferedTemp;
        }
    }
    void onAttackCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile, u32 attackKind, u32 attackSituation)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            if (attackSituation == fighterHooks::as_AttackerFighter)
            {
                walljumpSpent &= ~(1 << fighterPlayerNo);
            }

            soModuleAccesser* at_moduleAccesser = attacker->m_moduleAccesser;
            soModuleAccesser* tr_moduleAccesser = target->m_moduleAccesser;
            
            if (target->m_taskCategory == gfTask::Category_Fighter 
                && tr_moduleAccesser->m_enumerationStart->m_workManageModule->isFlag(parryActiveBit))
            {
                g_ecMgr->endEffect(mechUtil::reqCenteredGraphic(target, ef_ptc_pokemon_fire_04, 1.0f, 0));

                soStatusModule* at_statusModule = at_moduleAccesser->m_enumerationStart->m_statusModule;
                SituationKind at_situation = at_moduleAccesser->m_enumerationStart->m_situationModule->getKind();
                if (at_situation == Situation_Ground)
                {
                    at_statusModule->changeStatusRequest(Fighter::Status_FuraFura_End, at_moduleAccesser);
                    for (u32 i = Fighter::Status_Transition_Term_Group_Chk_Ground_Special; i <= Fighter::Status_Transition_Term_Group_Chk_Ground; i++)
                    {
                        at_statusModule->unableTransitionTermGroup(i);
                    }
                    at_moduleAccesser->m_enumerationStart->m_motionModule->setRate((attackSituation == fighterHooks::as_AttackerFighter) ? 0.333f : 0.5f);
                }
                else if (at_situation == Situation_Air)
                {
                    at_statusModule->changeStatusRequest(Fighter::Status_Fall_Special, at_moduleAccesser);
                }
            }
        }
    }

    asm void shieldHijackHook()
    {
        lis r12, setShieldSize@ha;
        lfs f1, setShieldSize@l(r12);
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_FighterOnAttackCB = (fighterHooks::FighterOnAttackCB)onAttackCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);

        // 0x80874C88: 0x54 bytes into symbol "setShieldScale/[ftStatusUniqProcessGuardFunc]/ft_status_u" @ 0x80874C34
        SyringeCore::syInlineHookRel(0x16A274, shieldHijackHook, Modules::SORA_MELEE);
    }
}