#include <st/st_utility.h>
#include <ft/ft_external_value_accesser.h>
#include "actionableAirdodge.h"

using namespace codeMenu;
namespace actionableAirdodge
{
    char outputTag[] = "[actionableAirdodge] ";
    const u32 airdodgeTimerVar = 0x20000001;

    enum playerFlags
    {
        pf_DodgeSpent = 0x00,
        pf__COUNT
    };
    u8 perPlayerFlags[pf__COUNT];

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount && mechHub::getPassiveMechanicEnabled(fighterPlayerNo, mechHub::pmid_ACTI_WAVEDASH))
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;

            // First, grab the flags byte from storage, since we need to be able to track this across multiple frames.
            const u32 playerBit = 1 << fighterPlayerNo;
            u32 dodgeSpentTemp = perPlayerFlags[pf_DodgeSpent];

            u32 currStatus = statusModule->getStatusKind();

            // If we're currently in jumpsquat...
            if (currStatus == Fighter::Status_Escape_Air)
            {
                int airdodgeTimer = workManageModule->getInt(airdodgeTimerVar);
                if (airdodgeTimer <= 0x00)
                {
                    dodgeSpentTemp |= playerBit;
                    airdodgeTimer--;
                }
                workManageModule->setInt(airdodgeTimer, airdodgeTimerVar);
                if (airdodgeTimer > -0x0A)
                {
                    Vec3f gravityMultiplier = { 0.0f, 0.0f, 0.0f };
                    moduleAccesser->m_enumerationStart->m_kineticModule->getEnergy(Fighter::Kinetic_Energy_Gravity)->mulAccel(&gravityMultiplier);
                }
                else
                {
                    workManageModule->setInt(0, airdodgeTimerVar);
                    soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                    soMotionChangeParam changeParam = { motionModule->getKind(), motionModule->getFrame(), 1.0f };
                    statusModule->changeStatus(Fighter::Status_Fall_Aerial, moduleAccesser);
                    motionModule->changeMotionRequest(&changeParam);
                }
            }

            u32 currSituation = moduleAccesser->m_enumerationStart->m_situationModule->getKind();
            
            if (currSituation == Situation_Air)
            {
                if (mechUtil::isDamageStatusKind(currStatus))
                {
                    dodgeSpentTemp &= ~playerBit;
                }
                else
                {
                    if (dodgeSpentTemp & playerBit)
                    {
                        statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Escape);
                    }
                }
            }
            else
            {
                dodgeSpentTemp &= ~playerBit;
            }

            perPlayerFlags[pf_DodgeSpent] = dodgeSpentTemp;
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
