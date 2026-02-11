#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "rm_hitfalling.h"

using namespace codeMenu;
namespace rmHitfalling
{
    char outputTag[] = "[RM_Hitfalling] ";

    void onUpdateCallback(Fighter* fighterIn)
    {
        // If we're in a valid port and have Hitfalling enabled...
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_HITFALLING))
        {
            // ... and we're performing an aerial...
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            u32 currStatus = moduleAccesser->m_enumerationStart->m_statusModule->getStatusKind();
            if (currStatus == Fighter::Status::Attack_Air)
            {
                // ... check if we're in hitstop, and have just vertically flicked the stick.
                soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
                if ((controllerModule->getFlickY() < 5) && fighterIn->m_moduleAccesser->m_enumerationStart->m_stopModule->isHit())
                {
                    // ... if the stick after that flick is below .60 units...
                    if (controllerModule->getStickY() < .60f)
                    {
                        // ... we've flicked downwards, so put us into fastfall!
                        moduleAccesser->m_enumerationStart->m_workManageModule->onFlag(Fighter::Status::Work_Flag_Reserve_Dive);
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
