#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "squatDodge.h"

using namespace codeMenu;
namespace squatDodge
{
    char outputTag[] = "[squatDodge] ";

    enum playerFlags
    {
        pf_DodgeBuffered = 0x00,
        pf__COUNT
    };
    u8 perPlayerFlags[pf__COUNT];

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SQUAT_DODGE))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            // First, grab the flags byte from storage, since we need to be able to track this across multiple frames.
            const u32 playerBit = 1 << fighterPlayerNo;
            u32 dodgeBufferedTemp = perPlayerFlags[pf_DodgeBuffered];
            // If we're currently in jumpsquat...
            if (statusModule->getStatusKind() == Fighter::Status_Jump_Squat)
            {
                // ... and a guard button (not Z, specifically) was triggered this frame...
                ipPadButton buttonTrigger = moduleAccesser->m_enumerationStart->m_controllerModule->getTrigger();
                if (buttonTrigger.m_guard && !buttonTrigger.m_attack)
                {
                    // ... then flag that we've buffered an airdodge.
                    dodgeBufferedTemp |= playerBit;
                }
                // Then on the final frame of jumpsquat, if we've buffered an airdodge...
                if (mechUtil::currAnimProgress(fighterIn) == 1.0f && dodgeBufferedTemp & playerBit)
                {
                    // ... transition immediately into airdodge...
                    statusModule->changeStatusRequest(Fighter::Status_Escape_Air, moduleAccesser);
                    // ... and unset this port's buffer flag.
                    dodgeBufferedTemp &= ~playerBit;
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
