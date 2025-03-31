#include <ft/fighter.h>
#include <st/st_utility.h>
#include "squatDodge.h"

using namespace codeMenu;
namespace squatDodge
{
    char outputTag[] = "[squatDodge] ";

    const float attachDistance = 5.0f;
    Vec3f searchVector = { 0.0f, -attachDistance, 0.0f };

    u8 dodgeSpent;
    u8 dodgeBuffered;
    u8 walljumpSpent;

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SQUAT_DODGE))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            u32 currStatus = statusModule->getStatusKind();

            u32 playerBit = 1 << fighterPlayerNo;
            u32 dodgeSpentTemp = dodgeSpent;
            u32 dodgeBufferedTemp = dodgeBuffered;
            u32 walljumpSpentTemp = walljumpSpent;


            soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
            if (currStatus == Fighter::Status_Jump_Squat)
            {
                if (moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger().m_guard)
                {
                    dodgeBufferedTemp |= playerBit;
                }
            }
            else if (currStatus == Fighter::Status_Jump && statusModule->getPrevStatusKind(0) == Fighter::Status_Jump_Squat)
            {
                if ((dodgeBufferedTemp & playerBit))
                {
                    statusModule->changeStatus(Fighter::Status_Escape_Air, moduleAccesser);
                }
                dodgeBufferedTemp &= ~playerBit;
            }
            else if (currStatus == Fighter::Status_Escape_Air)
            {
                float currAnimProgress = mechUtil::currAnimProgress(fighterIn);
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
                else if (currAnimProgress > 0.5f)
                {
                    dodgeSpentTemp |= playerBit;
                    statusModule->changeStatus(Fighter::Status_Fall_Aerial, moduleAccesser);
                }
            }
            else if (currStatus == Fighter::Status_Wall_Jump)
            {
                walljumpSpentTemp |= playerBit;
            }
            else
            {
                dodgeBufferedTemp &= ~playerBit;
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