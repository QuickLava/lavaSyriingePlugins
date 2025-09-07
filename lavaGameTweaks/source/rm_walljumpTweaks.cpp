#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "rm_walljumpTweaks.h"

using namespace codeMenu;
namespace rmWalljumpTweaks
{
    char outputTag[] = "[RM_WalljumpTweaks] ";
    char walljumpStatusFmt[] = "%sWalljump Status: %s\n";
    
    enum playerFlags
    {
        pf_WalljumpAllowedNative = 0x00,
        pf_WalljumpOutOfSpecialEnabled,
        pf__COUNT
    };
    u8 perPlayerFlags[pf__COUNT];
    enum walljumpSituation
    {
        wt_INVALID = -1,
        wt_NONE = 0,
        wt_LEFT,
        wt_RIGHT,
        wt__COUNT
    };
    u8 perPlayerPrevWalljump[fighterHooks::maxFighterCount];
    walljumpSituation decideWalljumpSituation(Fighter* fighterIn)
    {
        bool situationValid = 0;
        walljumpSituation currWalljumpSituation = wt_NONE;

        // If we're a fighter in a valid slot, in the air, on a character that's allowed to walljump...
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
        if (fighterPlayerNo < fighterHooks::maxFighterCount 
            && moduleAccesser->m_enumerationStart->m_situationModule->getKind() == Situation_Air
            && ftValueAccesser::getConstantInt(moduleAccesser, ftValueAccesser::Info_Param_Int_Wall_Jump_Type, 0))
        {
            // ... process validity of current walljump situation!
            walljumpSituation prevWalljumpSituation = (walljumpSituation)perPlayerPrevWalljump[fighterPlayerNo];
            soGroundModule* groundModule = moduleAccesser->m_enumerationStart->m_groundModule;
            // If we're touching a wall to our left...
            if (groundModule->isTouch(2, 0))
            {
                // ... then we can currently jump right.
                currWalljumpSituation = wt_RIGHT;
            }
            // If instead we're touching to our right...
            else if (groundModule->isTouch(4, 0))
            {
                // ... then we can currently jump left.
                currWalljumpSituation = wt_LEFT;
            }
            // Our current walljump at base is valid if we're touching a wall.
            situationValid = currWalljumpSituation >= wt_NONE;

            // If we're touching a wall, but the direction restriction is active...
            if ((currWalljumpSituation > wt_NONE) && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_SAME_DIR_RESTRICTION))
            {
                // ... our current walljump situation is valid if current situation would have us jump in a different direction than we did last.
                situationValid = currWalljumpSituation != prevWalljumpSituation;
                // If current situation is invalid...
                if (!situationValid)
                {
                    // ... then report that we're within the lockout still!
                    OSReport_N("%sDirection Restriction Active: Situation %d -> %d\N", outputTag, prevWalljumpSituation, currWalljumpSituation);
                }
            }

            // If we're still valid, and we have button walljumping enabled.
            if (situationValid && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_BUTTON))
            {
                // ... we need to also check how many frames it's been since we dropped from ledge.
                u32 framesSinceLedgeGrab =
                    ftValueAccesser::getConstantInt(moduleAccesser, ftValueAccesser::Common_Param_Int_Cliff_No_Catch_Frame, 0)
                    - moduleAccesser->m_enumerationStart->m_workManageModule->getInt(Fighter::Instance_Work_Int_Cliff_No_Catch_Frame);
                // Our situation is valid if we're out of the lockout window.
                situationValid = framesSinceLedgeGrab >= 8;
                // If it's invalid...
                if (!situationValid)
                {
                    // ... then report that we're within the lockout stil!
                    OSReport_N("Lockout Active\N", outputTag);
                }
            }
        }

        return situationValid ? currWalljumpSituation : wt_INVALID;
    }
    void tryButtonWalljump(Fighter* fighterIn)
    {
        // If this fighter is in a valid slot, can walljump, *and* their current state allows for it..
        walljumpSituation currWalljumpSituation = decideWalljumpSituation(fighterIn);
        if (currWalljumpSituation != wt_NONE)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soModuleEnumeration* moduleEnum = moduleAccesser->m_enumerationStart;

            // Set finalDir based on the direction reported by the validation function.
            float finalDir = (currWalljumpSituation == wt_RIGHT) ? 1.0f : -1.0f;
            // Finally get the absolute x position of the control stick.
            float stickXPos = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_X, 0);
            // If the stick is being held left or right, multiply finalDir into the current stick x position. If the result is less than 0.0f,
            // then we were holding into the wall we were touching...
            if (fabsf(stickXPos) > 0.15f && (finalDir * stickXPos) < 0.0f)
            {
                // ... then our conditions are satisfied!
                OSReport_N("%sButtonJump Allowed!\n", outputTag, finalDir);
                // In that case, check that we've just pressed the jump button...
                soControllerModule* controllerModule = moduleEnum->m_controllerModule;
                if (controllerModule->getTrigger().m_jump)
                {
                    // ... and if so, set our LR to our final direction...
                    soPostureModule* postureModule = moduleEnum->m_postureModule;
                    postureModule->setLr(finalDir);
                    // ... and perform the Wall Jump!
                    moduleEnum->m_statusModule->changeStatusRequest(Fighter::Status_Wall_Jump, moduleAccesser);
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

            const u32 playerBit = 1 << fighterPlayerNo;
            u32 walljumpNativeTemp = perPlayerFlags[pf_WalljumpAllowedNative];
            u32 specialWalljumpTemp = perPlayerFlags[pf_WalljumpOutOfSpecialEnabled];

            u32 currStatus = statusModule->getStatusKind();

            // If we're in the air...
            u32 currSituation = moduleAccesser->m_enumerationStart->m_situationModule->getKind();
            if (currSituation == Situation_Air)
            {
                // ... handle walljump availability.
                // First, if we've hit someone this frame...
                if (moduleAccesser->m_enumerationStart->m_collisionAttackModule->isInflict())
                {
                    // ... restore walljump, and log it!
                    perPlayerPrevWalljump[fighterPlayerNo] = wt_NONE;
                    OSReport_N("%s[P%d] Walljump Restored after Attack!\n", outputTag, fighterPlayerNo);
                }
                // Check current walljump situation...
                walljumpSituation currWalljumpSituation = decideWalljumpSituation(fighterIn);
                // ... and if no walljump is allowed...
                if (currWalljumpSituation == wt_INVALID)
                {
                    // ... then disable the transition entirely.
                    statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                }
                // Otherwise, do our normal checks.
                else
                {
                    // If the current action is one that's allowed to walljump natively...
                    if (walljumpNativeTemp & playerBit)
                    {
                        // ... enable walljumping.
                        statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                    }
                    // If Walljump out of Special(Fall) is enabled...
                    else if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_FROM_SPECIAL))
                    {
                        // ... check if we're currently in a special without the walljump flag set.
                        if (currStatus > Fighter::Status_Test_Motion && (specialWalljumpTemp & playerBit) == 0)
                        {
                            // If so, check the current state to see if we should enable the flag...
                            soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                            if (!motionModule->isLooped() && motionModule->getFrame() >= 15.0f)
                            {
                                float animProgress = mechUtil::currAnimProgress(fighterIn);
                                float ySpeed =
                                    ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Kinetic_Sum_Speed_Y, 0);
                                if (animProgress >= 0.80f
                                    || (animProgress >= 0.50f && ySpeed < -0.5f && !fighterIn->isEnableCancel()))
                                {
                                    // ... and if so, turn the flag on and enable walljumping!
                                    specialWalljumpTemp |= playerBit;
                                    statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                                }
                            }
                        }
                    }
                    // Lastly, check if Button Walljumping is enabled...
                    if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_BUTTON))
                    {
                        // ... and attempt one if so.
                        tryButtonWalljump(fighterIn);
                    }
                }
            }

            perPlayerFlags[pf_WalljumpOutOfSpecialEnabled] = specialWalljumpTemp;
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
            u32 walljumpNativeTemp = perPlayerFlags[pf_WalljumpAllowedNative];
            u32 specialWalljumpTemp = perPlayerFlags[pf_WalljumpOutOfSpecialEnabled];

            // Update the native walljump cancel flag for the current action.
            walljumpNativeTemp &= ~playerBit;
            walljumpNativeTemp |= statusModule->isEnableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump) << fighterPlayerNo;

            // If we're in the air...
            u32 currStatus = statusModule->getStatusKind();
            u32 currSituation = moduleAccesser->m_enumerationStart->m_situationModule->getKind();
            if (currSituation == Situation_Air)
            {
                // ... and Walljump out of Special Fall is on...
                if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_FROM_SPECIAL))
                {
                    // ... check if we're transitioning into a special...
                    if (currStatus > Fighter::Status_Test_Motion)
                    {
                        // ... while the walljump flag is enabled, and if so...
                        if (specialWalljumpTemp & playerBit)
                        {
                            // ... enable walljumping in the new action, and signal that it's allowed!
                            OSReport_N("%sSpecialWalljumpOn\n", outputTag);
                            statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                        }
                    }
                    // Otherwise, if the action we're transitioning into isn't a special...
                    else
                    {
                        // ... turn off the specials walljump flag.
                        specialWalljumpTemp &= ~playerBit;
                        // Additionally, check if we're transitioning into Special Fall.
                        if (currStatus == Fighter::Status_Fall_Special)
                        {
                            // If so, enable that transition group.
                            statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                        }
                    }
                }
                // Additionally, if Walljump Once Per Airtime is on...
                if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_SAME_DIR_RESTRICTION))
                {
                    // ... check if we've just entered the Walljump status...
                    if (currStatus == Fighter::Status_Wall_Jump)
                    {
                        perPlayerPrevWalljump[fighterPlayerNo] = (moduleAccesser->m_enumerationStart->m_postureModule->getLr() > 0.0f) ? wt_RIGHT : wt_LEFT;
                    }
                    // Alternatively, if we've just been hit...
                    else if (mechUtil::isDamageStatusKind(currStatus))
                    {
                        // ... restore walljump, and log it!
                        perPlayerPrevWalljump[fighterPlayerNo] = wt_NONE;
                        OSReport_N("%s[P%d] Walljump Restored after Damage!\n", outputTag, fighterPlayerNo);
                    }
                    // Lastly, if we're either not touching a wall, or actively disallowed from walljumping...
                    if (decideWalljumpSituation(fighterIn) <= wt_NONE)
                    {
                        // ... then disable the transition group.
                        statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                    }
                }

            }
            // Otherwise, if we're not in the air...
            else
            {
                // ... reset both flags.
                specialWalljumpTemp &= ~playerBit;
                perPlayerPrevWalljump[fighterPlayerNo] = wt_NONE;
            }

            perPlayerFlags[pf_WalljumpAllowedNative] = walljumpNativeTemp;
            perPlayerFlags[pf_WalljumpOutOfSpecialEnabled] = specialWalljumpTemp;
        }
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_FighterOnStatusChangeCB = (fighterHooks::FighterOnStatusChangeCB)onStatusChangeCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}
