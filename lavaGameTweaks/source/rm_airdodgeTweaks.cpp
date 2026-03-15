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
    Vec3f searchVector = Vec3f(0.0f, -attachDistance, 0.0f);
    // Length of time before you become actionable after the end of airdodge movement.
    const int inactionabilityLen = 0x8;
    // Motion change param for when we swap to Fall_Special after Actionable Airdodge.
    soMotionChangeParam fallSpecialChangeParam = { Fighter::Motion::Fall_Special, 0.0f, 1.0f };

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
            if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_HORI_WAVEDASH) && currStatus == Fighter::Status::Escape_Air)
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
                if (currStatus == Fighter::Status::Jump_Squat)
                {
                    // ... and a guard button (not Z, specifically) was triggered this frame...
                    ipPadButton buttonTrigger = moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger();
                    if (buttonTrigger.m_guard && !buttonTrigger.m_attack)
                    {
                        // ... then flag that we've buffered an airdodge.
                        dodgeBufferedTemp |= playerBit;
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
                if (currStatus == Fighter::Status::Escape_Air)
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
                    // Once we reach the end of the inactionability period...
                    if (airdodgeTimer == -inactionabilityLen)
                    {
                        // ... re-enable all transitions but Air Dodge.
                        soTransitionModule* transitionModule = statusModule->m_transitionModule;
                        for (int i = Fighter::Status::Transition::Group_Chk_Air_Landing; i <= Fighter::Status::Transition::Group_Chk_Air_Jump_Aerial; i++)
                        {
                            if (i == Fighter::Status::Transition::Group_Chk_Air_Escape) continue;
                            transitionModule->enableTermGroup(i);
                        }
                    }
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
                            statusModule->unableTransitionTermGroup(Fighter::Status::Transition::Group_Chk_Air_Escape);
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

    u32 transitionOverrideCallback(Fighter* fighterIn, int transitionTermIDIn, u32 targetActionIn)
    {
        u32 result = targetActionIn;
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            // If we have Airdodge out of JumpSquat enabled...
            if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SQUAT_DODGE))
            {
                // ... grab the flags byte from storage...
                u32 dodgeBufferedTemp = perPlayerFlags[pf_DodgeBuffered];
                // Then, if we're trying to transition to jump via Misc Group interrupt term, and we did buffer...
                if (targetActionIn == Fighter::Status::Jump && transitionTermIDIn == -1 && (dodgeBufferedTemp & (1 << fighterPlayerNo)))
                {
                    // ... override transition to go immediately into airdodge...
                    result = Fighter::Status::Escape_Air;
                    // ... and log that the buffer was triggered!
                    OSReport_N("%sAirdodge Buffer Activated!\n", outputTag);
                }
            }
            // If we have Actionable Airdodges enabled...
            if (mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_ACTI_WAVEDASH))
            {
                // ... and if we're transitioning into Fall_Special from Airdodge...
                soStatusModuleImpl* statusModule = (soStatusModuleImpl*)fighterIn->m_moduleAccesser->m_enumerationStart->m_statusModule;
                if (targetActionIn == Fighter::Status::Fall_Special && transitionTermIDIn == -1 && (statusModule->getStatusKind() == Fighter::Status::Escape_Air))
                {
                    // ... override that transition to go into Fall_Aerial instead (to maintain actionability, and avoid jump lockout).
                    result = Fighter::Status::Fall_Aerial;
                    OSReport_N("%sAirdodge Fall_Special Override Activated!\n", outputTag);
                }
            }
        }
        return result;
    }

    void onStatusChangeCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_ACTI_WAVEDASH))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;

            u32 currStatus = statusModule->getStatusKind();
            
            // If we're in tumble...
            if (currStatus == Fighter::Status::Damage_Fall)
            {
                // ... enable air dodge!
                statusModule->enableTransitionTermGroup(Fighter::Status::Transition::Group_Chk_Air_Escape);
            }
            // Otherwise, if we're in Fall_Aerial following Airdodge...
            else if (statusModule->getStatusKind() == Fighter::Status::Fall_Aerial && statusModule->getPrevStatusKind(0) == Fighter::Status::Escape_Air)
            {
                // ... that's cuz of our intervention! Swap to Fall_Special anim, since that's the anim Escape_Air is visually meant to flow into.
                soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                motionModule->changeMotionRequest(&fallSpecialChangeParam);
                // Additionally, Grab the workManageModule...
                soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;
                // ... and set backwards and forwards secondary animation indexes to their Fall_Special versions!
                // Note: This is normally set in ftStatusUniqProcessFall's initStatus, we need to override it cuz we're faking the anim.
                //  Don't need to actually do the tweening ourselves though, since that's handled in the associated execStatus function.
                workManageModule->setInt(Fighter::Motion::Fall_Special_B, 0x20000001);
                workManageModule->setInt(Fighter::Motion::Fall_Special_F, 0x20000000);
            }
        }
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_TransitionOverrideCB = (fighterHooks::TransitionTermEventCB)transitionOverrideCallback,
        .m_FighterOnStatusChangeCB = (fighterHooks::FighterOnStatusChangeCB)onStatusChangeCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}
