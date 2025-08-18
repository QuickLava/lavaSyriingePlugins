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
                    // ... determine what specific subaction we're in, to control how our transitions work.
                    soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                    u32 currMotion = motionModule->getKind();
                    // And determine whether or not we've actually hit anything in the current jab string...
                    if (moduleAccesser->m_enumerationStart->m_collisionAttackModule->isInflictStatus() & 0b110)
                    {
                        // ... and enable reverses if so!
                        tiltCancelReverseTemp |= playerBit;
                    }
                    // If we're at least 20% through the current action, hit the A button, and are in Jab 1 or 2...
                    if (mechUtil::currAnimProgress(fighterIn) >= 0.20
                        && moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger().m_attack
                        && (currMotion == Fighter::Motion_Attack_11 || currMotion == Fighter::Motion_Attack_12))
                    {
                        // ... log that we've successfully triggered a tilt cancel!
                        OSReport_N("%sTiltCancel Activated\n", outputTag);
                        // Grab the X and Y positions of the stick.
                        float stickX = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_X_Lr, 0);
                        float stickY = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_Y, 0);
                        // ... and give F-Tilt if you held forward...
                        if (stickX > ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Common_Param_Float_Attack_S3_Stick_X, 0))
                        {
                            statusModule->changeStatusRequest(Fighter::Status_Attack_S3, moduleAccesser);
                        }
                        // ... U-Tilt if you were holding up...
                        else if (stickY > ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Common_Param_Float_Attack_Hi3_Stick_Y, 0))
                        {
                            statusModule->changeStatusRequest(Fighter::Status_Attack_Hi3, moduleAccesser);
                        }
                        // ... and down-tilt if you were holding down.
                        else if (stickY < ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Common_Param_Float_Attack_Lw3_Stick_Y, 0))
                        {
                            statusModule->changeStatusRequest(Fighter::Status_Attack_Lw3, moduleAccesser);
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