#include <sy_compat.h>
#include <os/OSError.h>
#include <OS/OSCache.h>
#include <revolution/FA.h>
#include <gf/gf_scene.h>
#include <gf/gf_heap_manager.h>
#include <memory.h>
#include <modules.h>
#include <string.h>
#include <logUtils.h>
#include <ft/fighter.h>
#include <st/st_utility.h>

namespace fighterLayer {

    const char outputTag[] = "[fighterLayer] ";

    void(*_initStatusSpecialHi)(u32, soModuleAccesser*);
    void initStatusSpecialHi(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        OSReport_N("%sPreSpecialHi!\n", outputTag);
    }
    void(*_execStatusSpecialHi)(u32, soModuleAccesser*);
    void execStatusSpecialHi(u32 unk, soModuleAccesser* moduleAccesser)
    {
        OSReport_N("%sExecSpecialHi! Param_1: %08X\n", outputTag, unk);
        
        soStatusModule* statusModule = moduleAccesser->m_enumerationStart->m_statusModule;
        soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
        u32 currStatus = statusModule->getStatusKind();
        switch (currStatus)
        {
            case 0x11E:
            {
                if (moduleAccesser->m_enumerationStart->m_situationModule->getKind() == Situation_Air
                    && ftValueAccesser::getValueFloat(moduleAccesser, ftValueAccesser::Var_Float_Kinetic_Sum_Speed_Y, 0) > 1.0f
                    && ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_Y, 0) < 0.0f)
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
                    Vec3f searchVector(0.0f, -8.0f, 0.0f);
                    if (stRayCheck(&currPos, &searchVector, &lineIDOut, &hitPosOut, &normalVecOut, 1, 0, 1))
                    {
                        // If so, report the distance we snapped from for logging purposes...
                        float distanceFromGround = currPos.m_y - hitPosOut.m_y;
                        OSReport_N("%sDistanceFromGround: %.3f\n", outputTag, distanceFromGround);
                        // ... and attach to the ground!
                        statusModule->changeStatusRequest(0x11F, moduleAccesser);
                    }
                }
            }
        }
    }
    //void(*_processFixPosition)(u32, soModuleAccesser*);
    //void processFixPosition(Fighter* fighterIn)
    //{
    //    _processFixPosition(fighterIn);
    //}
    //void(*_notifyEventChangeStatus)(Fighter*, u32, u32, int, soModuleAccesser*);
    //void notifyEventChangeStatus(Fighter* param_1, u32 statusKind, u32 param_3, int param_4, soModuleAccesser* moduleAccesser)
    //{
    //    _notifyEventChangeStatus(param_1, statusKind, param_3, param_4, moduleAccesser);
    //}
    void Init()
    {
        // 0x8108a88c + 0xC8DC: ftIkeStatusUniqProcessSpecialHi__initStatus
        SyringeCompat::syReplaceFuncRel(0xC8DC,
            reinterpret_cast<void*>(initStatusSpecialHi),
            reinterpret_cast<void**>(&_initStatusSpecialHi),
            Modules::FT_IKE);
        SyringeCompat::syReplaceFuncRel(0xCCC4,
            reinterpret_cast<void*>(execStatusSpecialHi),
            reinterpret_cast<void**>(&_execStatusSpecialHi),
            Modules::FT_IKE);


        //SyringeCompat::syReplaceFuncRel(0x7718,
        //    reinterpret_cast<void*>(processFixPosition),
        //    reinterpret_cast<void**>(&_processFixPosition),
        //    Modules::FT_IKE);
        //SyringeCompat::syReplaceFuncRel(0x8F90,
        //    reinterpret_cast<void*>(notifyEventChangeStatus),
        //    reinterpret_cast<void**>(&_notifyEventChangeStatus),
        //    Modules::FT_IKE);
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}