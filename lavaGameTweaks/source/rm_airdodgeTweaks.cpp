#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "rm_airdodgeTweaks.h"

using namespace codeMenu;
namespace rmAirdodgeTweaks
{
    char outputTag[] = "[RM_AirdodgeTweaks] ";
    const u32 airdodgeTimerVar = 0x20000001;

    // Distance from which we're allowed to snap to the ground when airdodging.
    const float attachDistance = 5.0f;
    Vec3f searchVector = { 0.0f, -attachDistance, 0.0f };

    enum playerFlags
    {
        pf_DodgeBuffered = 0x00,
        pf_DodgeSpent,
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
            
            // Get the relevant bitmask for this player's flag bytes.
            const u32 playerBit = 1 << fighterPlayerNo;
            u32 currStatus = statusModule->getStatusKind();

            // If Horizontal Wavedashing is enabled, and we're currently Air Dodging...
            if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_HORI_WAVEDASH) && currStatus == Fighter::Status_Escape_Air)
            {
                // ... verify that we're past the we're at least 25% of the way through the animation.
                if (mechUtil::currAnimProgress(fighterIn) <= 0.25)
                {
                    // If we are, we're allowed to snap to ground. Grab the groundModule...
                    soGroundModule* groundModule = moduleAccesser->m_enumerationStart->m_groundModule;
                    // ... and our current position, from that.
                    Vec3f currPos;
                    Vec2f* currPosCastAddr = (Vec2f*)(&currPos);
                    *currPosCastAddr = groundModule->getDownPos(0);
                    currPos.m_z = 0.0f;
                    // Raycast down from our location the distance defined by attachDistance, and if we hit, check if our Y-Velocity is < 0.0.
                    int lineIDOut;
                    Vec3f hitPosOut;
                    Vec3f normalVecOut;
                    if (stRayCheck(&currPos, &searchVector, &lineIDOut, &hitPosOut, &normalVecOut, 1, 0, 1)
                        && ftValueAccesser::getValueFloat(moduleAccesser, ftValueAccesser::Var_Float_Kinetic_Sum_Speed_Y, 0) <= 0.0f)
                    {
                        // If so, report the distance we snapped from for logging purposes...
                        float distanceFromGround = currPos.m_y - hitPosOut.m_y;
                        OSReport_N("%sDistanceFromGround: %.3f\n", outputTag, distanceFromGround);
                        // ... and attach to the ground!
                        groundModule->attachGround(0);
                        groundModule->apply();
                    }
                }
            }
            
            // If Bufferable Airdodging is Enabled...
            if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SQUAT_DODGE))
            {
                // Grab the flags byte from storage, since we need to be able to track this across multiple frames.
                u32 dodgeBufferedTemp = perPlayerFlags[pf_DodgeBuffered];
                // If we're currently in jumpsquat...
                if (currStatus == Fighter::Status_Jump_Squat)
                {
                    // ... and a guard button (not Z, specifically) was triggered this frame...
                    ipPadButton buttonTrigger = moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger();
                    if (buttonTrigger.m_guard && !buttonTrigger.m_attack)
                    {
                        // ... then flag that we've buffered an airdodge.
                        dodgeBufferedTemp |= playerBit;
                    }
                    // Then on the final frame of jumpsquat, if we've buffered an airdodge...
                    if (mechUtil::currAnimProgress(fighterIn) == 1.0f && dodgeBufferedTemp & playerBit)
                    {
                        // ... transition immediately into airdodge...
                        statusModule->changeStatusRequest(Fighter::Status_Escape_Air, moduleAccesser);
                        // ... and unset this port's buffer flag.
                        dodgeBufferedTemp &= ~playerBit;
                    }
                }
                // Otherwise, if we're not in jumpsquat...
                else
                {
                    // ... also unset the flag bit, just as a precaution.
                    dodgeBufferedTemp &= ~playerBit;
                }
                // Write the modified temporary flag byte back into storage so we can reference it next frame.
                perPlayerFlags[pf_DodgeBuffered] = dodgeBufferedTemp;
            }

            // Lastly, if Actionable Airdodges are Enabled...
            if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_ACTI_WAVEDASH))
            {
                // Grab the dodge-spent flag byte.
                u32 dodgeSpentTemp = perPlayerFlags[pf_DodgeSpent];

                // If we're currently in air dodge...
                if (currStatus == Fighter::Status_Escape_Air)
                {
                    // ... grab the P+ airdodge momentum timer.
                    int airdodgeTimer = workManageModule->getInt(airdodgeTimerVar);
                    // If the airdodge's normal phase is over (ie. the timer is at 0 or below)...
                    if (airdodgeTimer <= 0x00)
                    {
                        // ... consider our airdodge spent...
                        dodgeSpentTemp |= playerBit;
                        // ... and handle continuing to decrement it, since we need to continue keeping track of frames.
                        airdodgeTimer--;
                    }
                    // Commit that change to the actual variable so it sticks around into the next frame.
                    workManageModule->setInt(airdodgeTimer, airdodgeTimerVar);
                    // As long as we're less than 10 frames past the end of the normal phase...
                    if (airdodgeTimer > -0x0A)
                    {
                        // ... nulify our our gravity, so we get that Rivals-like hang in the air.
                        Vec3f gravityMultiplier = { 0.0f, 0.0f, 0.0f };
                        moduleAccesser->m_enumerationStart->m_kineticModule->getEnergy(Fighter::Kinetic_Energy_Gravity)->mulAccel(&gravityMultiplier);
                    }
                    // Otherwise...
                    else
                    {
                        // ... reset our airdodge timer...
                        workManageModule->setInt(0, airdodgeTimerVar);
                        // ... and capture the current animation state, so we can resume it after transitioning to regular fall.
                        soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                        soMotionChangeParam changeParam = { motionModule->getKind(), motionModule->getFrame(), 1.0f };
                        // Perform the status change...
                        statusModule->changeStatus(Fighter::Status_Fall_Aerial, moduleAccesser);
                        // ... then restore the previous animation state.
                        motionModule->changeMotionRequest(&changeParam);
                    }
                }
                // Otherwise, if we're in tumble...
                else if (currStatus == Fighter::Status_Damage_Fall)
                {
                    // ... enable air dodge!
                    statusModule->enableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Escape);
                }
                // Lastly, if we're in the air...
                u32 currSituation = moduleAccesser->m_enumerationStart->m_situationModule->getKind();
                if (currSituation == Situation_Air)
                {
                    // ... and we're currently being hit...
                    if (mechUtil::isDamageStatusKind(currStatus))
                    {
                        // ... un-spend our airdodge.
                        dodgeSpentTemp &= ~playerBit;
                    }
                    // Otherwise...
                    else
                    {
                        // ... if the airdodge spent flag is on...
                        if (dodgeSpentTemp & playerBit)
                        {
                            // ... then enforce that we're not allowed to airdodge by disabling the transition.
                            statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Escape);
                        }
                    }
                }
                // Otherwise, if we're not in the air, un-spend our airdodge.
                else
                {
                    dodgeSpentTemp &= ~playerBit;
                }

                // Store any changes to the flags byte.
                perPlayerFlags[pf_DodgeSpent] = dodgeSpentTemp;
            }
        }
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}
