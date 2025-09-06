#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "rm_parries.h"

using namespace codeMenu;
namespace rmParries
{
    char outputTag[] = "[RM_Parries] ";

    const u32 guardOffCancelFrameVar = 0x20000005;
    const u32 parryActiveBit = 0x2200001E;
    const u32 parrySuccessBit = 0x2200001F;
    const u32 parrySuccessInvincFrames = 90;
    const u32 parrySuccessEndlagFrames = 5;
    const float parryActiveAnimSpeed = 0.4f;
    const float parryInactiveAnimSpeed = 0.2f;
    const float parryActivityProportion = 0.5f;
    const float parryActivityProportionRecpr = (1.0f / parryActivityProportion);

    enum playerFlags
    {
        pf_ParryBuffered = 0x00,
        pf_ParrySuffered,
        pf__COUNT
    };
    u8 perPlayerFlags[pf__COUNT];

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            // Fetch our character's current action.
            u32 currStatus = statusModule->getStatusKind();
            switch (currStatus)
            {
                // If we've got shield up, then we need to check for parry input.
                case Fighter::Status_Guard_On: case Fighter::Status_Guard:
                {
                    // Fetch the controller module...
                    soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
                    // ... and if Parries are enabled, *and* we've just pressed the Special button...
                    if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SHIELD_PARRY) && controllerModule->getTrigger().m_special)
                    {
                        // ... then set the flag which indicates we've buffered a parry, and trigger a change to Guard Off, where we'll do parry logic!
                        perPlayerFlags[pf_ParryBuffered] |= (1 << fighterPlayerNo);
                        statusModule->changeStatusRequest(Fighter::Status_Guard_Off, moduleAccesser);
                    }
                    break;
                }
                // If instead we're already in GuardOff...
                case Fighter::Status_Guard_Off:
                {
                    // ... and we're in the action because we're performing a parry...
                    if (workManageModule->isFlag(parryActiveBit))
                    {
                        // ... then we need to manage the ongoing effects of the parry!
                        // First, check if our reflect box was hit...
                        soCollisionShieldModuleImpl* collisionReflectModule = (soCollisionShieldModuleImpl*)moduleAccesser->m_enumerationStart->m_collisionReflectorModule;
                        if (collisionReflectModule->m_collisionOccurred != 0x00)
                        {
                            // ... and if so, flag that the parry was successful!
                            workManageModule->onFlag(parrySuccessBit);
                        }

                        // Next, initialize parry flash color...
                        GXColor parryFlashRGBA = { 0x08, 0x08, 0x00, 0x00 };
                        float animProgress = mechUtil::currAnimProgress(fighterIn);
                        // ... and if we're still within the active portion of the parry (and haven't already succeded in it)
                        if (animProgress < parryActivityProportion && !workManageModule->isFlag(parrySuccessBit))
                        {
                            // ... modulate the flash alpha based on progress through the active period of the parry.
                            parryFlashRGBA.a = 256.0f * (1.0f - (parryActivityProportionRecpr * animProgress));
                        }
                        // Otherwise, if we're either out of the active period or have succeeded the parry...
                        else
                        {
                            // ... then we need to begin turning off the activity effects.
                            // First, re-enable taking damage and remove the parry armor...
                            soDamageModule* damageModule = moduleAccesser->m_enumerationStart->m_damageModule;
                            damageModule->setDamageMul(1.0f);
                            damageModule->resetNoReactionModeStatus();
                            // ... turn off the activity flag...
                            workManageModule->offFlag(parryActiveBit);
                            // ... and turn off the reflect status.
                            moduleAccesser->m_enumerationStart->m_collisionReflectorModule->setStatusAll(0, 1);
                            // Then, if the parry was *successful*...
                            if (workManageModule->isFlag(parrySuccessBit))
                            {
                                // ... set variable to re-engage cancel loop in Fighter.pac, to allow acting out of parry.
                                workManageModule->setInt(parrySuccessEndlagFrames, guardOffCancelFrameVar);
                                // Additionally turn on the invincibility period...
                                soCollisionHitModule* hitModule = moduleAccesser->m_enumerationStart->m_collisionHitModule;
                                hitModule->setInvincibleFrameGlobal(parrySuccessInvincFrames, 1, 0);
                                // ... and play the success sound effect.
                                mechUtil::playSE(fighterIn, snd_se_Audience_Kansei_l);
                            }
                            // Otherwise, if we ended the parry without succeeding...
                            else
                            {
                                // ... then apply the failure speed.
                                soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                                motionModule->setRate(parryInactiveAnimSpeed);
                                // Also play our failure sound effect.
                                mechUtil::playSE(fighterIn, snd_se_Audience_Zannen);
                            }
                        }

                        // Finally, apply the flash as we configured it above.
                        moduleAccesser->m_enumerationStart->m_colorBlendModule->setFlash(parryFlashRGBA, 1);
                    }
                    break;
                }
            }
        }
    }
    void onStatusChangeCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            // Additionally, we'll need to flag that the attacker has *been* parried. Fetch relevant flag byte, and determine relevant bit.
            const u32 playerBit = 1 << fighterPlayerNo;
            u32 parrySufferedTemp = perPlayerFlags[pf_ParrySuffered];

            // Check what state we're in.
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)fighterIn->m_moduleAccesser->m_enumerationStart->m_statusModule;
            switch (statusModule->getStatusKind())
            {
                // If we're in GuardOff...
                case Fighter::Status_Guard_Off:
                {
                    // ... check if we arrived here with a parry buffered. If so...
                    u32 parryBufferedTemp = perPlayerFlags[pf_ParryBuffered];
                    if (parryBufferedTemp & playerBit)
                    {
                        // ... turn the flag off, and initialize our effects.
                        parryBufferedTemp &= ~playerBit;

                        // First, apply our speed modifier.
                        moduleAccesser->m_enumerationStart->m_motionModule->setRate(parryActiveAnimSpeed);

                        // Next, pulse the fire effect to get just the initial circle...
                        g_ecMgr->endEffect(mechUtil::reqCenteredGraphic(fighterIn, ef_ptc_pokemon_fire_04, 1.0f, 0));
                        // ... spawn a smoke effect at our feet.
                        moduleAccesser->m_enumerationStart->m_effectModule->req(ef_ptc_common_vertical_smoke_b, mechUtil::sbid_TransN,
                            &mechUtil::zeroVec, &mechUtil::zeroVec, 1.0f, &mechUtil::zeroVec, &mechUtil::zeroVec, 0, 0);

                        // Next, give ourselves knockback armor, and set our damage multiplier to 0x to avoid taking damage.
                        soDamageModule* damageModule = moduleAccesser->m_enumerationStart->m_damageModule;
                        damageModule->setNoReactionModeStatus(500.0f, -1.0f, 2);
                        damageModule->setDamageMul(0.0f);

                        // Additionally, enable the reflector status so we bounce back any projectiles.
                        moduleAccesser->m_enumerationStart->m_collisionReflectorModule->setStatusAll(1, 1);

                        // Prepare our PSA variables to control behavior.
                        soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;
                        // First, set the parry activity bit so that the Update callback handles the parry's ongoing effects.
                        workManageModule->onFlag(parryActiveBit);
                        // Set RA-Basic[3] to signal to Fighter.pac that we shouldn't be able to cancel out of this state.
                        workManageModule->setInt(0x00, 0x20000003);
                        // Also set RA-Basic[5] to functionally disengage the cancel loop in Fighter.pac to again avoid enabling any cancels.
                        workManageModule->setInt(0xFF, 0x20000005);
                        // Lastly, explicitly disable jumping out of the parry.
                        statusModule->unableTransitionTerm(Fighter::Status_Transition_Term_Cont_Jump_Squat, 0);
                        statusModule->unableTransitionTerm(Fighter::Status_Transition_Term_Cont_Jump_Squat_Button, 0);
                    }
                    // Store back the edited flag byte to enforce any changes from this function call.
                    perPlayerFlags[pf_ParryBuffered] = parryBufferedTemp;
                    break;
                }
                // If we're entering FuraFura...
                case Fighter::Status_FuraFura:
                {
                    // ... and arrives there *because* they were parried...
                    if (parrySufferedTemp & playerBit)
                    {
                        // ... jump straight to FuraFuraEnd.
                        statusModule->changeStatus(Fighter::Status_FuraFura_End, moduleAccesser);
                    }
                    break;
                }
                // Additionally, FuraFura_End is the state we use for opponent parry lag; if a fighter enters that state...
                case Fighter::Status_FuraFura_End:
                {
                    // ... and arrives there *because* they were parried...
                    if (parrySufferedTemp & playerBit)
                    {
                        // Turn off the flag and store it back!
                        perPlayerFlags[pf_ParrySuffered] = parrySufferedTemp & ~playerBit;
                        // Additionally, disable all of their relevant ground interrupts.
                        for (u32 i = Fighter::Status_Transition_Term_Group_Chk_Ground_Special; i <= Fighter::Status_Transition_Term_Group_Chk_Ground; i++)
                        {
                            statusModule->unableTransitionTermGroup(i);
                        }
                        // Lastly, slow their animation speed to lengthen the punishment period.
                        moduleAccesser->m_enumerationStart->m_motionModule->setRate(0.5f);
                    }
                    // Intentionally, also flow down into the below to trigger the parry overlay!
                    // Note: we intentionally left parrySufferedTemp unmodified cuz we need it to trigger when we flow into the below case!
                }
                // If we're in any of the intermediate states experienced after getting parried in the air...
                case Fighter::Status_Shield_Break_Fall: case Fighter::Status_Shield_Break_Down: case Fighter::Status_FuraFura_Stand:
                {
                    // ... and we arrived there *because* we were parried...
                    if (parrySufferedTemp & playerBit)
                    {
                        // ...apply the darkening effect.
                        GXColor parryFlashRGBA = { 0x08, 0x08, 0x00, 0xA0 };
                        moduleAccesser->m_enumerationStart->m_colorBlendModule->setFlash(parryFlashRGBA, 1);
                    }
                    break;
                }
                // If we're not in any of the above Actions...
                default:
                {
                    // ... and parries are enabled for this fighter... 
                    if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SHIELD_PARRY))
                    {
                        // ... we'll check for the Frame-Perfect input. First, grab the transition module...
                        soTransitionModule* transitionModule = statusModule->m_transitionModule;
                        // ... then check if to get to the current action we used a Ground Special transition, and also pressed shield this frame.
                        soTransitionInfo* previousTransition = transitionModule->getLastTransitionInfo();
                        if (previousTransition->m_groupId == Fighter::Status_Transition_Term_Group_Chk_Ground_Special
                            && moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger().m_guard)
                        {
                            // If so, that means pressed Shield + Special on the same frame, which is our parry input!
                            // Also check if the previous state generally allows us to Special Cancel (to screen out things like magic series cancels).
                            u32 prevStatus = statusModule->getPrevStatusKind(0);
                            if (prevStatus <= Fighter::Status_Landing_Fall_Special)
                            {
                                // If that holds true, then log that a parry input occurred!
                                soSituationModule* situationModule = moduleAccesser->m_enumerationStart->m_situationModule;
                                OSReport_N("%sFrame Perfect Parry Input! Situation Pre:%d", outputTag, situationModule->getKind());
                                // Then, prepare to perform the parry. Set the buffered flag so that the rest of the code handles it correctly...
                                perPlayerFlags[pf_ParryBuffered] |= playerBit;
                                // ... ensure we're attached to the ground, cuz some specials put you in the air immediately...
                                soGroundModule* groundModule = moduleAccesser->m_enumerationStart->m_groundModule;
                                groundModule->attachGround(0);
                                groundModule->apply(); 
                                situationModule->update();
                                OSReport_N(", Situation Post:%d\n", situationModule->getKind());
                                // ... and finally trigger the change!
                                statusModule->changeStatusRequest(Fighter::Status_Guard_Off, moduleAccesser);
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    void onAttackCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile, u32 attackKind, u32 attackSituation)
    {
        // Handle actual hit detecting for the parry. First, if the thing being hit isn't a Fighter, it can't have parried, so exit.
        if (target->m_taskCategory != gfTask::Category_Fighter) return;
        // Next, verify that the player being attacked actually has parries enabled.
        u32 targetPlayerNo = fighterHooks::getFighterPlayerNo((Fighter*)target);
        if (targetPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(targetPlayerNo, mechHub::pmid_SHIELD_PARRY))
        {
            // If so, handle the hit!
            soModuleAccesser* at_moduleAccesser = attacker->m_moduleAccesser;
            soModuleAccesser* tr_moduleAccesser = target->m_moduleAccesser;

            // Check if the target has the parry activity flag set, and is in GuardOff.
            soWorkManageModule* tr_workManageModule = tr_moduleAccesser->m_enumerationStart->m_workManageModule;
            if (tr_workManageModule->isFlag(parryActiveBit)
                && tr_moduleAccesser->m_enumerationStart->m_statusModule->getStatusKind() == Fighter::Status_Guard_Off)
            {
                // If they are, signal that they've successfully parried!
                tr_workManageModule->onFlag(parrySuccessBit);
                
                // Next, set the attacker's state to enforce the parry lag!
                soStatusModule* at_statusModule = at_moduleAccesser->m_enumerationStart->m_statusModule;
                SituationKind at_situation = at_moduleAccesser->m_enumerationStart->m_situationModule->getKind();
                // Initialize the attackers targeted end state to just their current one, since if they just jabbed we don't wanna change state.
                u32 endState = at_statusModule->getStatusKind();
                // If they're on the ground and landed anything stronger than a jab...
                if (at_situation == Situation_Ground && attackKind >= fighterHooks::ak_ATTACK_DASH)
                {
                    // ... set target state to FuraFuraEnd.
                    endState = Fighter::Status_FuraFura_End;
                }
                // Otherwise, if they're in the air...
                else if (at_situation == Situation_Air)
                {
                    // ... set target state to Shield Break Fall, so they lose actionability and transition into FuraFura on landing.
                    endState = Fighter::Status_Shield_Break_Fall;
                }
                // Lastly, flag that the attacker has *been* parried...
                perPlayerFlags[pf_ParrySuffered] |= (1 << fighterHooks::getFighterPlayerNo(attacker));
                // ... and put the character into the target end state.
                at_statusModule->changeStatusRequest(endState, at_moduleAccesser);
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
    }
}
