#include <modules.h>
#include <syWrapper.h>
#include <logUtils.h>
#include <ft/ft_manager.h>
#include "IASAOverlay.h"

namespace lavaIASAOverlay {

    const u8 groupsToCheck[] = {
        Fighter::Status_Transition_Group_Chk_Ground_Special,
        Fighter::Status_Transition_Group_Chk_Ground_Attack,
        Fighter::Status_Transition_Group_Chk_Ground_Guard,
        Fighter::Status_Transition_Group_Chk_Ground_Jump,
        Fighter::Status_Transition_Group_Chk_Air_Special,
        Fighter::Status_Transition_Group_Chk_Air_Escape,
        Fighter::Status_Transition_Group_Chk_Air_Attack,
        Fighter::Status_Transition_Group_Chk_Air_Jump_Aerial,
    };
    const u8 hardcodedActionable[] = {
        Fighter::Status_Down_Wait,
        Fighter::Status_Cliff_Wait,
        Fighter::Status_Air_Lasso_Hang,
        Fighter::Status_Slip_Wait,
    };

    void applyRelevantFlash()
    {
        // Use the function's moduleAccesser parameter to attempt to fetch the Fighter*...
        register soModuleAccesser* moduleAccesser;
        asm
        {
            mr moduleAccesser, r31;
        }
        Fighter* fighterIn = (Fighter*)moduleAccesser->m_stageObject;
        // ... and if it isn't actually a Fighter, abort.
        if (fighterIn->m_taskCategory != gfTask::Category_Fighter) return;

        // Otherwise, check if the fighter is in a valid port...
        u32 fighterPlayerNo = g_ftManager->getPlayerNo(fighterIn->m_entryId);
        if (fighterPlayerNo < 0x8)
        {
            // ... and if so, grab the Fighter's status module.
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;

            // Grab the current status...
            u32 currStatus = statusModule->getStatusKind();
            // ... and if we're in one of the basic grounded movement actions, just skip to avoid tinting on those actions.
            if (currStatus >= Fighter::Status_Walk && currStatus <= Fighter::Status_Turn_Run_Brake) return;

            // Initialize our variable for recording if we're actionable.
            bool actionable = 0;
            // While we haven't found an enabled one, loop through each group in our list of transition groups...
            for (u32 i = 0; !actionable && i < sizeof(groupsToCheck); i++)
            {
                // ... and record that we're actionable if that group is currently enabled.
                actionable = statusModule->isEnableTransitionTermGroup(groupsToCheck[i]);
            }
            // If none of the above groups were enabled, loop through the status IDs we've hardcoded as "actionable"...
            for (u32 i = 0; !actionable && i < sizeof(hardcodedActionable); i++)
            {
                // ... and record that we're actionable if our current status matches a found entry.
                actionable = currStatus == (u32)hardcodedActionable[i];
            }
            // If after the above, we're considered actionable...
            if (actionable)
            {
                // ... we're going to apply our overlay! Initiate the color just to pure Green.
                GXColor flashRGBA = { 0x00, 0xFF, 0x00, 0xC0 };
                // If we're in one of the normal attacking actions...
                if ((Fighter::Status_Attack <= currStatus && currStatus <= Fighter::Status_Attack_Air) || Fighter::Status_Test_Motion < currStatus)
                {
                    // ... then max the Red channel, so the color becomes Yellow while actionable out of an attack (eg. in IASA).
                    flashRGBA.r |= 0xFF;
                }
                // If alternatively we're in a damage action...
                else if (Fighter::Status_Damage <= currStatus && currStatus <= Fighter::Status_Down)
                {
                    // ... instead max the Blue channel, so the color becomes Teal while actionable out of knockback.
                    flashRGBA.b |= 0xFF;
                }
                // Apply the finalized color!
                moduleAccesser->m_enumerationStart->m_colorBlendModule->setFlash(flashRGBA, 1);
            }
        }
    }

    void Init()
    {
        // 0x8077F150 lands 0x8C bytes into symbol "checkTransition/[soStatusModuleImpl]/so_status_module_imp" @ 0x8077F0C4
        SyringeCompat::syInlineHookRel(0x7473C, reinterpret_cast<void*>(applyRelevantFlash), Modules::SORA_MELEE);
    }

    void Destroy(){}
}
