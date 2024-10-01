#include <os/OSError.h>
#include <sy_core.h>
#include <OS/OSCache.h>
#include <so/so_module_accesser.h>
#include <gr/gr_collision_shape.h>
#include <so/ground/so_ground_module_impl.h>
#include <so/work/so_work_manage_module_impl.h>
#include <so/kinetic/so_kinetic_module_impl.h>
#include <so/status/so_status_module_impl.h>
#include <so/damage/so_damage_module_impl.h>
#include <ac/ac_anim_cmd_impl.h>
#include <memory.h>
#include <modules.h>
#include <string.h>

namespace lavaPSAExtensions {

    const bool globalAllowDebugPrint = 1;
    const char outputTag[] = "[lavaPSAExtensions] ";

    void aerialInteruptPrevention()
    {
        const bool doDebugPrint = 1 & globalAllowDebugPrint;

        const char aerialNamePrefix[] = "AttackAir";
        const unsigned long RABitID = 0x22000019;

        register soModuleEnumeration* moduleEnum;
        asm
        {
            lwz moduleEnum, 0xD8(r31)
        }

        if (moduleEnum != NULL)
        {
            // If we're in an Action 0x33 (Aerial Attack), OR our current animation name starts with "AttackAir"...
            if (moduleEnum->m_statusModule->getStatusKind() == 0x33 ||
                strncmp(moduleEnum->m_motionModule->getName(), aerialNamePrefix, sizeof(aerialNamePrefix) - 1) == 0)
            {
                bool disablePlatLanding = 0;

                // ... and we're moving upwards...
                soInstanceAttribute flags; flags._0 = 0xFFFFu;
                float currentSpeedY = moduleEnum->m_kineticModule->getSumSpeed(&flags).m_y;
                if (currentSpeedY > 0.0f)
                {
                    // ... but our ECB is moving downwards...
                    grCollStatus* currCollStatus = moduleEnum->m_groundModule->getCollStatus(0x00);
                    float ECBShift = currCollStatus->m_currentCollShape->getDownPos().m_y - currCollStatus->m_prevCollShape->getDownPos().m_y;
                    if (ECBShift < 0.0f)
                    {
                        // ... then disable landing on platforms if RA-Bit[25] is set!
                        disablePlatLanding = moduleEnum->m_workManageModule->isFlag(RABitID);
                        // Do debug printing if desirable (this is optimized away when disabled).
                        if (doDebugPrint)
                        {
                            float currFrame = moduleEnum->m_motionModule->getFrame();
                            OSReport("%s[f%.0f, %s] Potential Aerial Interrupt Detected: %.3f Unit ECB Downshift!\n",
                                outputTag, currFrame, moduleEnum->m_motionModule->getName(), ECBShift - currentSpeedY);

                            if (disablePlatLanding)
                            {
                                OSReport("%s[f%.0f] Fallthrough Forced On!\n", outputTag, currFrame);
                            }
                        }
                    }
                }

                moduleEnum->m_groundModule->setPassableCheck(!disablePlatLanding, 0x00);
            }
        }
    };

    void Init()
    {
        // Note: 0x8070AA14 is SORA_MELEE base address
        SyringeCore::syInlineHookRel(0x12E680, reinterpret_cast<void*>(aerialInteruptPrevention), Modules::SORA_MELEE); // 0x80839094
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}