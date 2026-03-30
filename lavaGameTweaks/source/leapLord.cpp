#include "leapLord.h"

namespace leapLord
{
    char outputTag[] = "[leapLord] ";

    float chargeAmount[fighterHooks::maxFighterCount];
    const float minChargeMul = 0.25f;
    const float maxChargeMul = 1.50f;
    const float maxChargeLen = 40.0f;
    const float chargePerFrame = (maxChargeMul - minChargeMul) / maxChargeLen;

    const u16 chargeableStatuses[] = 
    { 
        Fighter::Status::Jump_Squat, Fighter::Status::Wall_Jump, Fighter::Status::Tread_Jump, 
        Fighter::Status::Cliff_Jump1, Fighter::Status::Cliff_Jump2, Fighter::Status::Cliff_Jump3, 
    };
    const u32 chargeableStatusCount = sizeof(chargeableStatuses) / sizeof(u16);

    bool isChargeableStatus(u32 statusIn)
    {
        for (u32 i = 0; i < chargeableStatusCount; i++)
        {
            if (statusIn == chargeableStatuses[i])
            {
                return 1;
            }
        }
        return 0;
    }

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;

            u32 currStatus = statusModule->getStatusKind();
            if (isChargeableStatus(currStatus))
            {
                bool promptedByButton = 0;
                int transitionTermID = statusModule->m_transitionModule->getLastTransitionInfo()->m_unitId;
                switch (currStatus)
                {
                    case Fighter::Status::Jump_Squat: 
                    { 
                        if (transitionTermID == Fighter::Status::Transition::Term_Cont_Jump_Squat_Button)
                        {
                            promptedByButton = 1;
                        }
                        break;
                    }
                    case Fighter::Status::Wall_Jump:
                    {
                        if (transitionTermID == Fighter::Status::Transition::Term_Passive_Wall_Jump_Button)
                        {
                            promptedByButton = 1;
                        }
                        break;
                    }
                    case Fighter::Status::Tread_Jump:
                    {
                        if (transitionTermID == Fighter::Status::Transition::Term_Cont_Tread_Jump_Button)
                        {
                            promptedByButton = 1;
                        }
                        break;
                    }
                    case Fighter::Status::Cliff_Jump1:
                    case Fighter::Status::Cliff_Jump2:
                    case Fighter::Status::Cliff_Jump3:
                    {
                        if (transitionTermID == Fighter::Status::Transition::Term_Cont_Cliff_Jump_Button)
                        {
                            promptedByButton = 1;
                        }
                        break;
                    }
                }

                soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
                ipPadButton padReleased = controllerModule->getRelease();

                if ((promptedByButton && (padReleased.m_mask & ipPadButton::MASK_JUMP))
                    || (!promptedByButton 
                        && controllerModule->getStickY() < ftValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Common_Param_Float_Jump_Stick_Y, 0)))
                {
                    soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                    motionModule->setRate(10.0f);
                }
                else
                {
                    chargeAmount[fighterPlayerNo] += chargePerFrame;
                }
            }
        }
    }
    void onStatusChangeCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;

            u32 currStatus = statusModule->getStatusKind();
            if (isChargeableStatus(currStatus))
            {
                chargeAmount[fighterPlayerNo] = minChargeMul;
                soMotionModule* motionModule = moduleAccesser->m_enumerationStart->m_motionModule;
                motionModule->setRate(motionModule->getEndFrame() / maxChargeLen);
            }
        }
    }

    void* _doParamOverride;
    asm void doParamOverride()
    {
         cmplwi r5, 3012;
         beq- Jump_Speed_X_Max;
         cmplwi r5, 3029;
         bne+ Jump_Speed_Y;
     Jump_Speed_X_Max:
         addi r5, r0, 3015;
         b Finish;
     Jump_Speed_Y:
         cmplwi r5, 3016;
         bne+ Tread_Jump_Speed_Y_Mul;
         addi r5, r0, 3013;
         b Finish;
     Tread_Jump_Speed_Y_Mul:
         cmplwi r5, 3020;
         bne+ Finish;
         addi r5, r0, 3019;
     Finish:
         lis r11, _doParamOverride@ha;
         lwz r11, _doParamOverride@l(r11);
         mtlr r11;
    }
    double applyParamModifiers(soModuleAccesser* moduleAccesserIn, u32 paramID)
    {   
        if (moduleAccesserIn->m_stageObject->m_taskCategory == gfTask::Category_Fighter)
        {
            Fighter* fighterIn = (Fighter*)moduleAccesserIn->m_stageObject;
            u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
            if (fighterPlayerNo < fighterHooks::maxFighterCount)
            {
                float currCharge = chargeAmount[fighterPlayerNo];

                soStatusModule* statusModule = moduleAccesserIn->m_enumerationStart->m_statusModule;
                soSituationModule* situationModule = moduleAccesserIn->m_enumerationStart->m_situationModule;

                u32 currStatus = statusModule->getStatusKind();
                u32 currSituation = situationModule->getKind();


                switch (paramID)
                {
                    case ftValueAccesser::Customize_Param_Float_Jump_Speed_Y:
                    case ftValueAccesser::Customize_Param_Float_Tread_Jump_Speed_Y_Mul:
                    case ftValueAccesser::Customize_Param_Float_Cliff_Jump_Speed_Y:
                    case ftValueAccesser::Customize_Param_Float_Passive_Wall_Jump_Speed_Y:
                    {
                        if (currSituation != Situation_Ground)
                        {
                            currCharge = (currCharge < 0.5f) ? 0.5f : currCharge;
                        }
                        valueIn *= currCharge;
                        break;
                    }
                    case ftValueAccesser::Customize_Param_Float_Air_Accel_X_Mul:
                    case ftValueAccesser::Customize_Param_Float_Air_Brake_X:
                    {
                        if (currStatus > Fighter::Status::Test_Motion && currSituation != Situation_Ground)
                        {
                            valueIn = 0.0f;
                        }
                        break;
                    }
                }
            }
        }
        
        return valueIn;
    }
    asm void _applyParamModifiers()
    {
        nofralloc
        mflr r31;                                       // Backup LR in a non-volatile register!
        mr r3, r29;
        mr r4, r30;
        bl applyParamModifiers;
        mtlr r31;
        blr;
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
        SyringeCompat::syReplaceFuncRel(0x8C314, reinterpret_cast<void*>(doParamOverride), &_doParamOverride, Modules::SORA_MELEE);
        SyringeCompat::syInlineHookRel(0x14B26C, reinterpret_cast<void*>(_applyParamModifiers), Modules::SORA_MELEE);
    }
}