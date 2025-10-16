#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "rm_tiltCancels.h"

using namespace codeMenu;
namespace rmTiltCancels
{
    char outputTag[] = "[RM_TiltCancels] ";

    enum playerFlags
    {
        pf_TiltCancelReverseEnabled = 0x00,
        pf__COUNT
    };
    u8 perPlayerFlags[pf__COUNT];

    const float attackS3StickThresh = 0.25f;
    const float attackHi3StickThresh = 0.25f;
    const float attackLw3StickThresh = -0.25f;

    u32 classifyRequestedTilt(soModuleAccesser* moduleAccesser)
    {
        u32 result = Fighter::Status_Test_Motion;
        float stickX = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_X_Lr, 0);
        float stickY = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_Y, 0);
        // ... and give F-Tilt if you held forward...
        if (stickX >= attackS3StickThresh)
        {
            result = Fighter::Status_Attack_S3;
        }
        // ... U-Tilt if you were holding up...
        else if (stickY >= attackHi3StickThresh)
        {
            result = Fighter::Status_Attack_Hi3;
        }
        // ... and down-tilt if you were holding down.
        else if (stickY <= attackLw3StickThresh)
        {
            result = Fighter::Status_Attack_Lw3;
        }
        return result;
    }

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_TILT_CANCELS))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;

            const u32 playerBit = 1 << fighterPlayerNo;
            u32 tiltCancelReverseTemp = perPlayerFlags[pf_TiltCancelReverseEnabled];

            // Grab the current Action we're in.
            u32 currStatus = statusModule->getStatusKind();
            switch (currStatus)
            {
                // If we're current jabbing...
                case Fighter::Status_Attack:
                {
                    // ... ddetermine whether or not we've actually hit anything in the current jab string...
                    if (moduleAccesser->m_enumerationStart->m_collisionAttackModule->isInflictStatus() & 0b110)
                    {
                        // ... and enable reverses if so!
                        tiltCancelReverseTemp |= playerBit;
                    }
                    // Piggyback off of the game's native tracking of jab combo progress!
                    // - RA-Bit[17] (0x22000011) in this context signals that the character can progress to their next jab stage.
                    // - RA-Bit[21] (0x22000015) signals that the next jab has been input (or buffered).
                    soWorkManageModule* workManagerModule = moduleAccesser->m_enumerationStart->m_workManageModule;
                    if (workManagerModule->isFlag(0x22000011) && workManagerModule->isFlag(0x22000015))
                    {
                        // If both are true, log that we've successfully triggered a tilt cancel scenario!
                        OSReport_N("%sTiltCancel Activated\n", outputTag);
                        u32 targetStatus = classifyRequestedTilt(moduleAccesser);
                        if (targetStatus != Fighter::Status_Test_Motion)
                        {
                            statusModule->changeStatusRequest(targetStatus, moduleAccesser);
                        }
                    }
                    break;
                }
                // Once in Up/Down Tilt...
                case Fighter::Status_Attack_Hi3: case Fighter::Status_Attack_Lw3:
                {
                    // ... grab the current Stick X.
                    float stickX = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_X_Lr, 0);
                    // If the tilt bit is currently set, and our stick is pointed at all backwards...
                    if ((tiltCancelReverseTemp & playerBit) && stickX <= -0.1f)
                    {
                        // ... then we've triggered a reverse! Log that it happened...
                        OSReport_N("%sReverse Requested\n", outputTag);
                        // ... and do the flip.
                        soPostureModule* postureModule = moduleAccesser->m_enumerationStart->m_postureModule;
                        postureModule->reverseLr();
                        postureModule->updateRotYLr();
                        statusModule->changeStatusRequest(currStatus, moduleAccesser);
                    }
                    // Unset the bit, finally.
                    tiltCancelReverseTemp &= ~playerBit;
                    break;
                }
            }

            perPlayerFlags[pf_TiltCancelReverseEnabled] = tiltCancelReverseTemp;
        }
    }
    u32 transitionOverrideCallback(Fighter* fighterIn, int transitionTermIDIn, u32 targetActionIn)
    {
        u32 result = targetActionIn;
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_TILT_CANCELS))
        {
            if (targetActionIn == Fighter::Status_Attack_100)
            {
                soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
                u32 requestedAction = classifyRequestedTilt(moduleAccesser);
                if (requestedAction != Fighter::Status_Test_Motion)
                {
                    // If both are true, log that we've successfully triggered a tilt cancel scenario!
                    OSReport_N("%sAttack100 Tilt Cancel Activated\n", outputTag);
                    result = requestedAction;
                }
            }
        }
        return result;
    }
#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_TransitionOverrideCB = (fighterHooks::TransitionTermEventCB)transitionOverrideCallback
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}