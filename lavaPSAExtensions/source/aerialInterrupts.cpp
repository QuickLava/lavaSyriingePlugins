#include <cstdlib>
#include <sy_compat.h>
#include <modules.h>
#include <ft/fighter.h>
#include <so/so_module_accesser.h>
#include <so/ground/so_ground_module_impl.h>
#include <so/work/so_work_manage_module_impl.h>
#include <so/kinetic/so_kinetic_module_impl.h>
#include <so/status/so_status_module_impl.h>
#include "aerialInterrupts.h"

namespace aerialInterrupts
{
    const char outputTag[] = "[aerialInterrupts] ";

    void aerialInteruptPrevention()
    {
        const unsigned long RABitID = 0x22000019;

        register soModuleEnumeration* moduleEnum;
        asm
        {
            lwz moduleEnum, 0xD8(r31)
        }

        if (moduleEnum != NULL)
        {
            // If we're in one of the Aerial Animations...
            u32 motionKind = moduleEnum->m_motionModule->getKind();
            if (motionKind >= Fighter::Motion::Attack_Air_N && motionKind <= Fighter::Motion::Attack_Air_Lw)
            {
                // ... ensure that landings are enabled by default.
                soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleEnum->m_statusModule;
                soTransitionModule* transitionModule = statusModule->m_transitionModule;
                transitionModule->enableTerm(Fighter::Status::Transition::Term_Landing_Attack_Air, 0);
                statusModule->enableTransitionTermGroup(Fighter::Status::Transition::Group_Chk_Air_Landing);

                // If however we're moving upwards...
                soInstanceAttribute flags; flags.unk0= 0xFFFFu;
                float currentSpeedY = moduleEnum->m_kineticModule->getSumSpeed(&flags).m_y;
                if (currentSpeedY > 0.0f)
                {
                    // ... and our ECB is moving downwards...
                    grCollStatus* currCollStatus = moduleEnum->m_groundModule->getCollStatus(0x00);
                    float ECBShift = currCollStatus->m_currentCollShape->getDownPos().m_y - currCollStatus->m_prevCollShape->getDownPos().m_y;
                    if (ECBShift < 0.0f)
                    {
                        // ... we're in a Potential Aerial Interrupt situation! Log that possibility...
                        float currFrame = moduleEnum->m_motionModule->getFrame();
                        OSReport_N("%s[f%.0f, %s] Potential Aerial Interrupt Detected: %.3f Unit ECB Downshift!\n",
                            outputTag, currFrame, moduleEnum->m_motionModule->getName(), ECBShift - currentSpeedY);

                        // ... and if the Aerial Interrupt protection bit is set...
                        if (moduleEnum->m_workManageModule->isFlag(RABitID))
                        {
                            // ... disable landing...
                            transitionModule->unableTerm(Fighter::Status::Transition::Term_Landing_Attack_Air, 0);
                            statusModule->unableTransitionTermGroup(Fighter::Status::Transition::Group_Chk_Air_Landing);
                            // ... and log that we've done so!
                            OSReport_N("%s[f%.0f] Landing Disabled!\n", outputTag, currFrame);
                        }
                    }
                }
            }
        }
    }
    void registerHooks()
    {
        SyringeCompat::syInlineHookRel(0x12E680, reinterpret_cast<void*>(aerialInteruptPrevention), Modules::SORA_MELEE); // 0x80839094
    }
}