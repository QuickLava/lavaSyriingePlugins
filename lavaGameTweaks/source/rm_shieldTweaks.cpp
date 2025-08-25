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

            u32 currStatus = statusModule->getStatusKind();
            switch (currStatus)
            {
                case Fighter::Status_Guard_On: case Fighter::Status_Guard: case Fighter::Status_Guard_Damage:
                {
                    float shieldRadius = ftValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Shield_Radius, 0);
                    Vec3f scaleBuf = { shieldRadius, shieldRadius, shieldRadius };

                    soModelModule* modelModule = moduleAccesser->m_enumerationStart->m_modelModule;
                    u32 nodeHandle = modelModule->getCorrectNodeId(300);
                    modelModule->setNodeScale(nodeHandle, &scaleBuf);
                    break;
                }
            }
        }
    }

    void onStatusChangeCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_SHIELD_BREAK_REDUCTION))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModule* statusModule = fighterIn->m_moduleAccesser->m_enumerationStart->m_statusModule;
            switch (statusModule->getStatusKind())
            {
                case Fighter::Status_FuraFura:
                {
                    statusModule->changeStatus(Fighter::Status_FuraFura_End, moduleAccesser);
                    break;
                }
                case Fighter::Status_FuraFura_End:
                {
                    for (u32 i = Fighter::Status_Transition_Term_Group_Chk_Ground_Special; i <= Fighter::Status_Transition_Term_Group_Chk_Ground; i++)
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
