#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "squatDodge.h"

using namespace codeMenu;
namespace squatDodge
{
    char outputTag[] = "[rivalsMode] ";

    const float attachDistance = 5.0f;
    Vec3f searchVector = { 0.0f, -attachDistance, 0.0f };

    const u32 parryActiveBit = 0x2200001E;
    const u32 parrySuccessBit = 0x2200001F;
    const u32 parrySuccessInvincFrames = 90;
    const u32 airdodgeTimerVar = 0x20000001;
    const float babyDashMinSpeed = 1.666f;
    const float setShieldSize = 60.0f;
    const float parryActiveAnimSpeed = 0.4f;
    const float parryInactiveAnimSpeed = 0.2f;
    const float parryActivityProportion = 0.5f;
    const float parryActivityProportionRecpr = (1.0f / parryActivityProportion);

    enum playerFlags
    {
        pf_DodgeSpent = 0x00,
        pf_DodgeBuffered,
        pf_ParryBuffered,
        pf_WallJumpSpent,
        pf_WalljumpOutOfSpecialEnabled,
        pf_TiltCancelReverseEnabled,
        pf__COUNT
    };
    u8 perPlayerFlags[pf__COUNT];

    void tryExtendedWalljump(Fighter* fighterIn)
    {
        soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
        soPostureModule* postureModule = moduleAccesser->m_enumerationStart->m_postureModule;
        float stickX = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Variable_Float_Controller_Stick_X, 0);
        if (fabs(stickX) >= 0.5f)
        {
            Vec3f wallCheckVec = { 5.0f, 0.0f, 0.0f };
            if (stickX < 0.0f)
            {
                wallCheckVec.m_x *= -1.0f;
            }
            int lineOut;
            Vec3f currPos = ftExternalValueAccesser::getHipPos(fighterIn);
            Vec3f hitPosOut;
            Vec3f normalVecOut;
            if (stRayCheck(&currPos, &wallCheckVec, &lineOut, &hitPosOut, &normalVecOut, 1, 0, 1))
            {
                float distanceFromWall = currPos.m_x - hitPosOut.m_x;
                OSReport_N("%sDistanceFromWall: %.3f\n", outputTag, distanceFromWall);

                soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
                if (controllerModule->getTrigger().m_jump)
                {
                    postureModule->setLr(distanceFromWall / fabs(distanceFromWall));
                    moduleAccesser->m_enumerationStart->m_statusModule->changeStatusRequest(Fighter::Status_Wall_Jump, moduleAccesser);
                }
            }
        }
    }

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            u32 currStatus = statusModule->getStatusKind();

            const u32 playerBit = 1 << fighterPlayerNo;
            u32 dodgeSpentTemp =    perPlayerFlags[pf_DodgeSpent];
            u32 dodgeBufferedTemp = perPlayerFlags[pf_DodgeBuffered];
            u32 walljumpSpentTemp = perPlayerFlags[pf_WallJumpSpent];
            u32 parryBufferedTemp = perPlayerFlags[pf_ParryBuffered];
            u32 specialWalljumpTemp = perPlayerFlags[pf_WalljumpOutOfSpecialEnabled];
            u32 tiltCancelReverseTemp = perPlayerFlags[pf_TiltCancelReverseEnabled];

            switch (currStatus)
            {
                case Fighter::Status_Jump_Squat:
                {
                    ipPadButton buttonTrigger = moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger();
                    if (buttonTrigger.m_guard && !buttonTrigger.m_attack)
                    {
                        dodgeBufferedTemp |= playerBit;
                    }
                    if (mechUtil::currAnimProgress(fighterIn) == 1.0f && dodgeBufferedTemp & playerBit)
                    {
                        statusModule->changeStatusRequest(Fighter::Status_Escape_Air, moduleAccesser);
                    }
                    break;
                }
                case Fighter::Status_Escape_Air:
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
                        if (stRayCheck(&currPos, &searchVector, &lineIDOut, &hitPosOut, &normalVecOut, 1, 0, 1)
                            && ftValueAccesser::getValueFloat(moduleAccesser, ftValueAccesser::Variable_Float_Kinetic_Sum_Speed_Y, 0) <= 0.0f)
                        {
                            float distanceFromGround = currPos.m_y - hitPosOut.m_y;
                            OSReport_N("%sDistanceFromGround: %.3f\n", outputTag, distanceFromGround);
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
                    if (airdodgeTimer > -0x0A)
                    {
                        Vec3f gravityMultiplier = { 0.0f, 0.0f, 0.0f };
                        moduleAccesser->m_enumerationStart->m_kineticModule->getEnergy(Fighter::Kinetic_Energy_Gravity)->mulAccel(&gravityMultiplier);
                    }
                    else
                    {
                        workManageModule->setInt(0, airdodgeTimerVar);
                        soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                        soMotionChangeParam changeParam = { motionModule->getKind(), motionModule->getFrame(), 1.0f };
                        statusModule->changeStatus(Fighter::Status_Fall_Aerial, moduleAccesser);
                        motionModule->changeMotionRequest(&changeParam);
                    }
                    break;
                }
                case Fighter::Status_Attack:
                {
                    soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                    u32 currMotion = motionModule->getKind();
                    if (moduleAccesser->m_enumerationStart->m_collisionAttackModule->isInflictStatus() & 0b110)
                    {
                        tiltCancelReverseTemp |= playerBit;
                    }
                    if (mechUtil::currAnimProgress(fighterIn) >= 0.20
                        && moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger().m_attack
                        && (currMotion == Fighter::Motion_Attack_11 || currMotion == Fighter::Motion_Attack_12))
                    {
                        float stickX = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Variable_Float_Controller_Stick_X_Lr, 0);
                        float stickY = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Variable_Float_Controller_Stick_Y, 0);
                        if (stickX > ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Common_Param_Float_Attack_S3_Stick_X, 0))
                        {
                            statusModule->changeStatusRequest(Fighter::Status_Attack_S3, moduleAccesser);
                        }
                        else if (stickY > ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Common_Param_Float_Attack_Hi3_Stick_Y, 0))
                        {
                            statusModule->changeStatusRequest(Fighter::Status_Attack_Hi3, moduleAccesser);
                        }
                        else if (stickY < ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Common_Param_Float_Attack_Lw3_Stick_Y, 0))
                        {
                            statusModule->changeStatusRequest(Fighter::Status_Attack_Lw3, moduleAccesser);
                        }
                    }
                    break;
                }
                case Fighter::Status_Attack_Hi3: case Fighter::Status_Attack_Lw3:
                {
                    float stickX = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Variable_Float_Controller_Stick_X_Lr, 0);
                    if ((tiltCancelReverseTemp & playerBit) && stickX <= -0.1f)
                    {
                        OSReport_N("%sReverse Requested\n", outputTag);
                        soPostureModule* postureModule = moduleAccesser->m_enumerationStart->m_postureModule;
                        postureModule->reverseLr();
                        postureModule->updateRotYLr();
                        statusModule->changeStatusRequest(currStatus, moduleAccesser);
                    }
                    tiltCancelReverseTemp &= ~playerBit;
                    break;
                }
                case Fighter::Status_FuraFura:
                {
                    statusModule->changeStatus(Fighter::Status_FuraFura_End, moduleAccesser);
                    break;
                }
                case Fighter::Status_Wall_Jump:
                {
                    walljumpSpentTemp |= playerBit;
                    specialWalljumpTemp &= ~playerBit;
                    break;
                }
                case Fighter::Status_Guard_On: case Fighter::Status_Guard:
                {
                    soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
                    if (controllerModule->getTrigger().m_special)
                    {
                        soTransitionModule* transitionModule = statusModule->m_transitionModule;
                        statusModule->changeStatusRequest(Fighter::Status_Guard_Off, moduleAccesser);
                        parryBufferedTemp |= playerBit;
                    }
                    break;
                }
                case Fighter::Status_Guard_Off:
                {
                    float animProgress = mechUtil::currAnimProgress(fighterIn);
                    if (workManageModule->isFlag(parryActiveBit))
                    {
                        GXColor parryFlashRGBA = { 0x08, 0x08, 0x00, 0x00 };
                        soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                        soDamageModule* damageModule = moduleAccesser->m_enumerationStart->m_damageModule;
                        soCollisionShieldModuleImpl* collisionReflectModule = (soCollisionShieldModuleImpl*)moduleAccesser->m_enumerationStart->m_collisionReflectorModule;

                        if (collisionReflectModule->m_collisionOccurred != 0x00)
                        {
                            workManageModule->onFlag(parrySuccessBit);
                        }

                        if (animProgress >= parryActivityProportion
                            || workManageModule->isFlag(parrySuccessBit))
                        {
                            damageModule->setDamageMul(1.0f);
                            damageModule->resetNoReactionModeStatus();
                            workManageModule->offFlag(parryActiveBit);
                            moduleAccesser->m_enumerationStart->m_collisionReflectorModule->setStatusAll(0, 1);
                            if (workManageModule->isFlag(parrySuccessBit))
                            {
                                workManageModule->setInt(0x5, 0x20000005);
                                soCollisionHitModule* hitModule = moduleAccesser->m_enumerationStart->m_collisionHitModule;
                                hitModule->setInvincibleFrameGlobal(parrySuccessInvincFrames, 1, 0);
                                mechUtil::playSE(fighterIn, snd_se_Audience_Kansei_l);
                            }
                            else
                            {
                                motionModule->setRate(parryInactiveAnimSpeed);
                                mechUtil::playSE(fighterIn, snd_se_Audience_Zannen);
                            }
                        }
                        else
                        {
                            parryFlashRGBA.a = 256.0f * (1.0f - (parryActivityProportionRecpr * animProgress));
                        }
                        moduleAccesser->m_enumerationStart->m_colorBlendModule->setFlash(parryFlashRGBA, 1);
                    }
                    break;
                }
                case Fighter::Status_Fall_Special:
                {
                    if ((walljumpSpentTemp & playerBit) == 0)
                    {
                        statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                    }
                    break;
                }
                case Fighter::Status_Damage_Fall:
                {
                    ipPadButton buttonTrigger = moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger();
                    if (buttonTrigger.m_guard)
                    {
                        statusModule->changeStatusRequest(Fighter::Status_Escape_Air, moduleAccesser);
                    }
                    break;
                }
                case Fighter::Status_Dash:
                {
                    if (moduleAccesser->m_enumerationStart->m_motionModule->getFrame() < 3.0f)
                    {
                        float stickXAbs = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Variable_Float_Controller_Stick_X_Abs, 0);
                        float stickYAbs = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Variable_Float_Controller_Stick_Y_Abs, 0);

                        if (stickXAbs < 0.2f && stickYAbs < 0.2f)
                        {
                            soKineticModule* kineticModule = moduleAccesser->m_enumerationStart->m_kineticModule;
                            statusModule->changeStatus(Fighter::Status_Wait, moduleAccesser);
                            kineticModule->clearSpeedAll();
                            float targetXSpeed = ftValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Dash_Speed, 0);
                            if (targetXSpeed < babyDashMinSpeed)
                            {
                                targetXSpeed = babyDashMinSpeed;
                            }
                            Vec3f newSpeed = { targetXSpeed, 0.0f, 0.0f };
                            kineticModule->addSpeed(&newSpeed, moduleAccesser);
                        }
                    }
                    break;
                }
                default:
                {
                    tiltCancelReverseTemp &= ~playerBit;
                    break;
                }
            }

            if (statusModule->getPrevStatusKind(0) == Fighter::Status_Jump_Squat)
            {
                dodgeBufferedTemp &= ~playerBit;
            }

            u32 currSituation = moduleAccesser->m_enumerationStart->m_situationModule->getKind();
            if (currSituation == Situation_Ground || currSituation == Situation_Cliff)
            {
                dodgeSpentTemp &= ~playerBit;
                walljumpSpentTemp &= ~playerBit;
                specialWalljumpTemp &= ~playerBit;
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
                    u32 framesSinceLedgeGrab = 
                        ftValueAccesser::getConstantInt(moduleAccesser, ftValueAccesser:: Common_Param_Int_Cliff_No_Catch_Frame, 0)
                        - workManageModule->getInt(Fighter::Instance_Work_Int_Cliff_No_Catch_Frame);
                    if ((walljumpSpentTemp & playerBit) || framesSinceLedgeGrab < 8)
                    {
                        OSReport_N("%sWalljump Disabled\n", outputTag);
                        statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                    }
                    else
                    {
                        if (currStatus > Fighter::Status_Test_Motion)
                        {
                            if (specialWalljumpTemp & playerBit)
                            {
                                OSReport_N("%sSpecialWalljumpOn\n", outputTag);
                                tryExtendedWalljump(fighterIn);
                                statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                            }
                            else
                            {
                                statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                                soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                                if (!motionModule->isLooped() && motionModule->getFrame() >= 15.0f)
                                {
                                    float animProgress = mechUtil::currAnimProgress(fighterIn);
                                    float ySpeed =
                                        ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Variable_Float_Kinetic_Sum_Speed_Y, 0);
                                    if (animProgress >= 0.80f 
                                        || (animProgress >= 0.50f && ySpeed < -0.5f && !fighterIn->isEnableCancel()))
                                    {
                                        specialWalljumpTemp |= playerBit;
                                    }
                                }
                            }
                        }
                        else
                        {
                            specialWalljumpTemp &= ~playerBit;
                            tryExtendedWalljump(fighterIn);
                        }
                    }
                }
            }

            perPlayerFlags[pf_DodgeSpent] = dodgeSpentTemp;
            perPlayerFlags[pf_DodgeBuffered] = dodgeBufferedTemp;
            perPlayerFlags[pf_WallJumpSpent] = walljumpSpentTemp;
            perPlayerFlags[pf_ParryBuffered] = parryBufferedTemp;
            perPlayerFlags[pf_WalljumpOutOfSpecialEnabled] = specialWalljumpTemp;
            perPlayerFlags[pf_TiltCancelReverseEnabled] = tiltCancelReverseTemp;
        }
    }
    void onStatusChangeCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModule* statusModule = fighterIn->m_moduleAccesser->m_enumerationStart->m_statusModule;

            const u32 playerBit = 1 << fighterPlayerNo;
            u32 walljumpSpentTemp = perPlayerFlags[pf_WallJumpSpent];
            u32 parryBufferedTemp = perPlayerFlags[pf_ParryBuffered];

            switch (statusModule->getStatusKind())
            {
                case Fighter::Status_Guard_Off:
                {
                    soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                    soDamageModule* damageModule = moduleAccesser->m_enumerationStart->m_damageModule;
                    soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

                    if (parryBufferedTemp & playerBit)
                    {
                        parryBufferedTemp &= ~playerBit;

                        motionModule->setRate(parryActiveAnimSpeed);
                        g_ecMgr->endEffect(mechUtil::reqCenteredGraphic(fighterIn, ef_ptc_pokemon_fire_04, 1.0f, 0));
                        moduleAccesser->m_enumerationStart->m_effectModule->req(ef_ptc_common_vertical_smoke_b, mechUtil::sbid_TransN,
                            &mechUtil::zeroVec, &mechUtil::zeroVec, 1.0f, &mechUtil::zeroVec, &mechUtil::zeroVec, 0, 0);
                        damageModule->setNoReactionModeStatus(500.0f, -1.0f, 2);
                        damageModule->setDamageMul(0.0f);
                        workManageModule->onFlag(parryActiveBit);
                        workManageModule->setInt(0x00, 0x20000003);
                        workManageModule->setInt(0xFF, 0x20000005);
                        statusModule->unableTransitionTerm(Fighter::Status_Transition_Term_Cont_Jump_Squat, 0);
                        statusModule->unableTransitionTerm(Fighter::Status_Transition_Term_Cont_Jump_Squat_Button, 0);
                        moduleAccesser->m_enumerationStart->m_collisionReflectorModule->setStatusAll(1, 1);
                    }
                    break;
                }
                case Fighter::Status_FuraFura_End:
                {
                    for (u32 i = Fighter::Status_Transition_Term_Group_Chk_Ground_Special; i <= Fighter::Status_Transition_Term_Group_Chk_Ground; i++)
                    {
                        statusModule->unableTransitionTermGroup(i);
                    }
                    break;
                }
            }
            
            u32 currSituation = moduleAccesser->m_enumerationStart->m_situationModule->getKind();
            if (currSituation == Situation_Air)
            {
                
                u32 framesTillLedgeGrabAllowed = 
                    moduleAccesser->m_enumerationStart->m_workManageModule->getInt(Fighter::Instance_Work_Int_Cliff_No_Catch_Frame);
                u32 framesSinceLedgeGrab =
                    ftValueAccesser::getConstantInt(moduleAccesser, ftValueAccesser::Common_Param_Int_Cliff_No_Catch_Frame, 0)
                    - framesTillLedgeGrabAllowed;
                if ((walljumpSpentTemp & playerBit) || framesSinceLedgeGrab < 8)
                {
                    OSReport_N("%sWalljump Disabled on StatusChange\n", outputTag);
                    statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                }
            }

            perPlayerFlags[pf_WallJumpSpent] = walljumpSpentTemp;
            perPlayerFlags[pf_ParryBuffered] = parryBufferedTemp;
        }
    }
    void onAttackCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile, u32 attackKind, u32 attackSituation)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            if (attackSituation == fighterHooks::as_AttackerFighter)
            {
                perPlayerFlags[pf_WallJumpSpent] &= ~(1 << fighterPlayerNo);
            }

            soModuleAccesser* at_moduleAccesser = attacker->m_moduleAccesser;
            soModuleAccesser* tr_moduleAccesser = target->m_moduleAccesser;
            
            soWorkManageModule* tr_workManageModule = tr_moduleAccesser->m_enumerationStart->m_workManageModule;
            if (target->m_taskCategory == gfTask::Category_Fighter 
                && tr_workManageModule->isFlag(parryActiveBit)
                && tr_moduleAccesser->m_enumerationStart->m_statusModule->getStatusKind() == Fighter::Status_Guard_Off)
            {
                tr_workManageModule->onFlag(parrySuccessBit);

                soStatusModule* at_statusModule = at_moduleAccesser->m_enumerationStart->m_statusModule;
                SituationKind at_situation = at_moduleAccesser->m_enumerationStart->m_situationModule->getKind();
                if (at_situation == Situation_Ground)
                {
                    if (attackKind >= fighterHooks::ak_ATTACK_DASH)
                    {
                        at_statusModule->changeStatusRequest(Fighter::Status_FuraFura_End, at_moduleAccesser);
                        GXColor parryFlashRGBA = { 0x08, 0x08, 0x00, 0xA0 };
                        at_moduleAccesser->m_enumerationStart->m_colorBlendModule->setFlash(parryFlashRGBA, 1);
                        at_moduleAccesser->m_enumerationStart->m_motionModule->setRate(0.5f);
                    }
                }
                else if (at_situation == Situation_Air)
                {
                    at_statusModule->changeStatusRequest(Fighter::Status_Shield_Break_Fall, at_moduleAccesser);
                }
            }
        }
    }

    asm void shieldHijackHook()
    {
        nofralloc;
        lis r12, setShieldSize@ha;
        lfs f1, setShieldSize@l(r12);
    }
    asm void walljumpHijackHook()
    {
        nofralloc;
        cmplwi r30, 0x5A95;
        bne end;
        lfs f30, 0x18(r13);
    end:
        blr;
    }

    u32 pointerBackup;
    void hitfallingHijackHook1()
    {
        register u32 funcPtr;
        asm
        {
            mr funcPtr, r3;
        }
        pointerBackup = funcPtr;
    }
    void hitfallingHijackHook2()
    {
        register soModuleAccesser* moduleAccesser;
        asm
        {
            mr moduleAccesser, r30;
        }
        OSReport_N("");

        if (pointerBackup != 0x80B897BC)
        {
            soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
            if (controllerModule->getFlickY() < 5)
            {
                float stickY = controllerModule->getStickY();
                if (stickY < .60f)
                {
                    moduleAccesser->m_enumerationStart->m_workManageModule->onFlag(0x22000002);
                }
            }
        }
    }


#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_FighterOnAttackCB = (fighterHooks::FighterOnAttackCB)onAttackCallback,
        .m_FighterOnStatusChangeCB = (fighterHooks::FighterOnStatusChangeCB)onStatusChangeCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);

        // 0x80874C88: 0x54 bytes into symbol "setShieldScale/[ftStatusUniqProcessGuardFunc]/ft_status_u" @ 0x80874C34
        SyringeCore::syInlineHookRel(0x16A274, shieldHijackHook, Modules::SORA_MELEE);

        // 0x807827C0: 0x324 bytes into symbol "checkEstablishSub/[soGeneralTermDisideModuleImpl]/so_gene" @ 0x8078249C
        SyringeCore::syInlineHookRel(0x77DAC, walljumpHijackHook, Modules::SORA_MELEE);

        // 0x8077E8D0 (G:0x8078C85C): 73EBC
        SyringeCore::syInlineHookRel(0x73EBC, hitfallingHijackHook1, Modules::SORA_MELEE);

        // 0x8077E8EC (G:0x8078C878): 
        SyringeCore::syInlineHookRel(0x73ED8, hitfallingHijackHook2, Modules::SORA_MELEE);
    }
}