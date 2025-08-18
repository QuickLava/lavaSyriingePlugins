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
        pf_WallJumpSpent,
        pf_WalljumpOutOfSpecialEnabled,
        pf__COUNT
    };
    u8 perPlayerFlags[pf__COUNT];

    void tryExtendedWalljump(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
        if (fighterPlayerNo < fighterHooks::maxFighterCount
            && moduleAccesser->m_enumerationStart->m_statusModule->isEnableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump))
        {
            soGroundModule* groundModule = moduleAccesser->m_enumerationStart->m_groundModule;
            float finalDir;
            if (groundModule->isTouch(2, 0))
            {
                finalDir = 1.0f;
            }
            else if (groundModule->isTouch(4, 0))
            {
                finalDir = -1.0f;
            }
            else
            {
                return;
            }

            OSReport_N("%sFinal ButtonJump Dir: %.0f\n", outputTag, finalDir);

            soPostureModule* postureModule = moduleAccesser->m_enumerationStart->m_postureModule;
            soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
            if (controllerModule->getTrigger().m_jump)
            {
                postureModule->setLr(finalDir);
                moduleAccesser->m_enumerationStart->m_statusModule->changeStatusRequest(Fighter::Status_Wall_Jump, moduleAccesser);
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
            u32 walljumpSpentTemp = perPlayerFlags[pf_WallJumpSpent];
            u32 specialWalljumpTemp = perPlayerFlags[pf_WalljumpOutOfSpecialEnabled];

            u32 currStatus = statusModule->getStatusKind();

            // If we're in the air...
            u32 currSituation = moduleAccesser->m_enumerationStart->m_situationModule->getKind();
            if (currSituation == Situation_Air)
            {
                // ... and Walljump out of Special Fall is enabled...
                if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_FROM_SPECIAL))
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
                // Additionally, check if Button Walljumping is enabled.
                if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_BUTTON))
                {
                    // If it is, we need to impose the ledge-drop jump lockout.
                    // First, check how many frames it's been since we dropped from ledge.
                    u32 framesSinceLedgeGrab =
                        ftValueAccesser::getConstantInt(moduleAccesser, ftValueAccesser::Common_Param_Int_Cliff_No_Catch_Frame, 0)
                        - workManageModule->getInt(Fighter::Instance_Work_Int_Cliff_No_Catch_Frame);
                    // If we're still within the defined lockout length...
                    if (framesSinceLedgeGrab < 8)
                    {
                        // ... then report that we're within the lockout still, and force-disable walljumping!
                        OSReport_N(walljumpStatusFmt, outputTag, "Lockout Active");
                        statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                    }
                    // Otherwise, if we're out of the lockout...
                    else if (framesSinceLedgeGrab == 8)
                    {
                        // ... and the current action is one that's allowed to walljump natively...
                        if (walljumpNativeTemp & playerBit)
                        {
                            // ... report, and re-enable walljumping.
                            OSReport_N(walljumpStatusFmt, outputTag, "Lockout Finished");
                            statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                        }
                    }

                    // Lastly, perform the button jump attempt.
                    tryExtendedWalljump(fighterIn);
                }
                // Lastly, if Walljump Once Per Airtime is on...
                if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_ONCE_PER_AIR))
                {
                    // ... then report that we're within the lockout still, and force-disable walljumping!
                    if (walljumpSpentTemp & playerBit)
                    {
                        OSReport_N(walljumpStatusFmt, outputTag, "Spent");
                        statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                    }
                }
            }
            // Otherwise, if we're in any other situation...
            else
            {
                // ... reset the spent and special walljump flags.
                walljumpSpentTemp &= ~playerBit;
                specialWalljumpTemp &= ~playerBit;
            }

            perPlayerFlags[pf_WallJumpSpent] = walljumpSpentTemp;
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
            u32 walljumpSpentTemp = perPlayerFlags[pf_WallJumpSpent];
            u32 specialWalljumpTemp = perPlayerFlags[pf_WalljumpOutOfSpecialEnabled];

            walljumpNativeTemp &= ~playerBit;
            if (statusModule->isEnableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump))
            {
                walljumpNativeTemp |= playerBit;
            }

            u32 currStatus = statusModule->getStatusKind();

            // If we're in the air...
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
                if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_WALLJUMP_ONCE_PER_AIR))
                {
                    // ... check if we've just entered the Walljump status...
                    if (currStatus == Fighter::Status_Wall_Jump)
                    {
                        // ... and mark that we've spent our walljump, if so.
                        walljumpSpentTemp |= playerBit;
                    }
                    // Alternatively, if we've just been hit...
                    else if (mechUtil::isDamageStatusKind(currStatus))
                    {
                        // ... then *unset* the spent bit instead!
                        walljumpSpentTemp &= ~playerBit;
                    }
                    // Lastly, regardless of what we're transitioning into, if the walljump spent flag is on...
                    if (walljumpSpentTemp & playerBit)
                    {
                        // ... disable walljumping in the new action, and signal that it's disallowed! 
                        OSReport_N(walljumpStatusFmt, outputTag, "Spent, Status Change");
                        statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Wall_Jump);
                    }
                }
            }
            // Otherwise, if we're not in the air...
            else
            {
                // ... reset both flags.
                walljumpSpentTemp &= ~playerBit;
                specialWalljumpTemp &= ~playerBit;
            }

            perPlayerFlags[pf_WalljumpAllowedNative] = walljumpNativeTemp;
            perPlayerFlags[pf_WallJumpSpent] = walljumpSpentTemp;
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
