#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "rm_babyDashes.h"

using namespace codeMenu;
namespace rmBabyDashes
{
    char outputTag[] = "[RM_BabyDashes] ";

    const float babyDashMinSpeed = 1.666f;
    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_BABY_DASH))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            u32 currStatus = statusModule->getStatusKind();

            if (currStatus == Fighter::Status::Dash)
            {
                if (moduleAccesser->m_enumerationStart->m_motionModule->getFrame() < 3.0f)
                {
                    float stickXAbs = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_X_Abs, 0);
                    float stickYAbs = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_Y_Abs, 0);

                    if (stickXAbs < 0.2f && stickYAbs < 0.2f)
                    {
                        soKineticModule* kineticModule = moduleAccesser->m_enumerationStart->m_kineticModule;
                        statusModule->changeStatus(Fighter::Status::Wait, moduleAccesser);
                        kineticModule->clearSpeedAll();
                        float targetXSpeed = ftValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Dash_Speed, 0);
                        if (targetXSpeed < babyDashMinSpeed)
                        {
                            targetXSpeed = babyDashMinSpeed;
                        }
                        Vec3f newSpeed(targetXSpeed, 0.0f, 0.0f);
                        kineticModule->addSpeed(&newSpeed, moduleAccesser);
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
