#include <st/st_utility.h>
#include "horizontalWavedashes.h"

using namespace codeMenu;
namespace horiWavedashes
{
    char outputTag[] = "[horiWavedashes] ";

    // Distance from which we're allowed to snap to the ground when airdodging.
    const float attachDistance = 5.0f;
    Vec3f searchVector = { 0.0f, -attachDistance, 0.0f };

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_HORI_WAVEDASH))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            // If we're currently Air Dodging...
            if (statusModule->getStatusKind() == Fighter::Status_Escape_Air)
            {
                // ... verify that we're past the we're at least 25% of the way through the animation.
                float currAnimProgress = mechUtil::currAnimProgress(fighterIn);
                if (currAnimProgress <= 0.25)
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
