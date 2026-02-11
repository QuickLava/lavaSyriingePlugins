#include <modules.h>
#include <sy_compat.h>
#include <logUtils.h>
#include <ft/ft_manager.h>
#include <_cmAddonInterface.h>
#include "IASAOverlay.h"

namespace lavaIASAOverlay {
    const char outputTag[] = "[IASAOverlay] ";
    const char addonShortName[] = "IASAOVER";
    enum _lineIDs
    {
        lid_WORKING_SPACE = 0,
        lid_ENABLED_P1,
        lid_ENABLED_P2,
        lid_ENABLED_P3,
        lid_ENABLED_P4,
        lid__COUNT
    };
    u32 indexBuffer[lid__COUNT];

    const u8 groupsToCheck[] = {
        Fighter::Status::Transition::Group_Chk_Ground_Special,
        Fighter::Status::Transition::Group_Chk_Ground_Attack,
        Fighter::Status::Transition::Group_Chk_Ground_Guard,
        Fighter::Status::Transition::Group_Chk_Ground_Jump,
        Fighter::Status::Transition::Group_Chk_Air_Special,
        Fighter::Status::Transition::Group_Chk_Air_Escape,
        Fighter::Status::Transition::Group_Chk_Air_Attack,
        Fighter::Status::Transition::Group_Chk_Air_Jump_Aerial,
    };
    const u8 hardcodedActionable[] = {
        Fighter::Status::Down_Wait,
        Fighter::Status::Cliff_Wait,
        Fighter::Status::Air_Lasso_Hang,
        Fighter::Status::Slip_Wait,
    };

    bool enabledForPlayer(u32 playerNo)
    {
        const codeMenu::cmSelectionLine** enabledLines = (const codeMenu::cmSelectionLine**)indexBuffer + 1;
        const codeMenu::cmSelectionLine* targetLine = enabledLines[playerNo];
        return (targetLine != NULL) ? targetLine->m_value : 1;
    }
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
        if (fighterPlayerNo < 0x4 && enabledForPlayer(fighterPlayerNo))
        {
            // ... and if so, grab the Fighter's status module.
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;

            // Grab the current status...
            u32 currStatus = statusModule->getStatusKind();
            // ... and if we're in one of the basic grounded movement actions, just skip to avoid tinting on those actions.
            if (currStatus >= Fighter::Status::Walk && currStatus <= Fighter::Status::Turn_Run_Brake) return;

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
                if ((Fighter::Status::Attack <= currStatus && currStatus <= Fighter::Status::Attack_Air) || Fighter::Status::Test_Motion < currStatus)
                {
                    // ... then max the Red channel, so the color becomes Yellow while actionable out of an attack (eg. in IASA).
                    flashRGBA.r |= 0xFF;
                }
                // If alternatively we're in a damage action...
                else if (Fighter::Status::Damage <= currStatus && currStatus <= Fighter::Status::Down)
                {
                    // ... instead max the Blue channel, so the color becomes Teal while actionable out of knockback.
                    flashRGBA.b |= 0xFF;
                }
                // Apply the finalized color!
                moduleAccesser->m_enumerationStart->m_colorBlendModule->setFlash(flashRGBA, 1);
            }
        }
    }

    bool populate()
    {
        bool result = codeMenu::loadCodeMenuAddonLOCsToBuffer(addonShortName, indexBuffer, lid__COUNT);
        if (result)
        {
            OSReport_N("%sSuccessfully Loaded Addon Index File to Buffer 0x%08X!\n", outputTag, indexBuffer);
        }
        else
        {
            OSReport_N("%sFailed to Load Addon Index File to Buffer!\n", outputTag);
        }
        return result;
    }
    void Init()
    {
        // Populate Code Menu Buffer
        populate();
        // 0x8077F150 lands 0x8C bytes into symbol "checkTransition/[soStatusModuleImpl]/so_status_module_imp" @ 0x8077F0C4
        SyringeCompat::syInlineHookRel(0x7473C, reinterpret_cast<void*>(applyRelevantFlash), Modules::SORA_MELEE);
    }

    void Destroy(){}
}
