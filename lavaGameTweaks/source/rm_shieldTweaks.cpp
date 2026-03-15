#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "rm_shieldTweaks.h"

using namespace codeMenu;
namespace rmShieldTweaks
{
    char outputTag[] = "[RM_ShieldTweaks] ";

    const float setShieldSize = 60.0f;

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SHIELD_SIZE_LOCK))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            // Grab the current status...
            u32 currStatus = statusModule->getStatusKind();
            switch (currStatus)
            {
                // ... and if we're currently in shield...
                case Fighter::Status::Guard_On: case Fighter::Status::Guard: case Fighter::Status::Guard_Damage:
                {
                    // ... fetch the shield's radius...
                    float shieldRadius = ftValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Shield_Radius, 0);
                    // ... and initialize a Vector3f with it.
                    Vec3f scaleBuf(shieldRadius, shieldRadius, shieldRadius);
                    // Then grab the shield node via the model module...
                    soModelModule* modelModule = moduleAccesser->m_enumerationStart->m_modelModule;
                    u32 nodeHandle = modelModule->getCorrectNodeId(300);
                    // ... and force its scale back to the default value.
                    modelModule->setNodeScale(nodeHandle, &scaleBuf);
                    break;
                }
            }
        }
    }

    void onStatusChangeCallback(Fighter* fighterIn)
    {
        // If the fighter is in a valid port, and has Shield Break Lag Reduction turned on...
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SHIELD_BREAK_REDUCTION))
        {
            // Check what state they're in.
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModule* statusModule = fighterIn->m_moduleAccesser->m_enumerationStart->m_statusModule;
            switch (statusModule->getStatusKind())
            {
                // If they're in the Shield Break Stun state (FuraFura)...
                case Fighter::Status::FuraFura:
                {
                    // ... force them into FuraFuraEnd instead!
                    statusModule->changeStatus(Fighter::Status::FuraFura_End, moduleAccesser);
                    break;
                }
                // If they're in FuraFuraEnd though...
                case Fighter::Status::FuraFura_End:
                {
                    // ... disable their grounded interrupts to force them through the full duration!
                    for (u32 i = Fighter::Status::Transition::Group_Chk_Ground_Special; i <= Fighter::Status::Transition::Group_Chk_Ground; i++)
                    {
                        statusModule->unableTransitionTermGroup(i);
                    }
                    break;
                }
            }
        }
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_FighterOnStatusChangeCB = (fighterHooks::FighterOnStatusChangeCB)onStatusChangeCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}
