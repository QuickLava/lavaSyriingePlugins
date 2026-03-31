#include "leapLord.h"

namespace leapLord
{
    char outputTag[] = "[leapLord] ";

    float chargeAmount[fighterHooks::maxFighterCount];
    const float minChargeMul = 0.00f;
    const float maxChargeMul = 1.50f;
    const float maxChargeLen = 40.0f;
    const float chargePerFrame = (maxChargeMul - minChargeMul) / maxChargeLen;

    soInstanceAttribute momentumReflectAttr = 1;

    const u16 chargeableStatuses[] = 
    { 
        Fighter::Status::Jump_Squat, Fighter::Status::Cliff_Jump1, Fighter::Status::Cliff_Jump2, Fighter::Status::Cliff_Jump3, 
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

    u8 reflectLockoutTimer[fighterHooks::maxFighterCount] = {};

    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;

            u8 currReflectTimer = reflectLockoutTimer[fighterPlayerNo];

            u32 currStatus = statusModule->getStatusKind();
            if (moduleAccesser->m_enumerationStart->m_situationModule->getKind() == Situation_Air)
            {
                if (currReflectTimer == 0
                    && moduleAccesser->m_enumerationStart->m_workManageModule->getInt(Fighter::Instance::Work::Int_Cliff_No_Catch_Frame) <= 0)
                {
                    soGroundModule* groundModule = moduleAccesser->m_enumerationStart->m_groundModule;

                    Vec3f reflectVec(0.0f, 0.0f, 0.0f);
                    float currXSpeed = ftValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Kinetic_Sum_Speed_X, 0);
                    u32 touchFlags = groundModule->getTouchFlag(0);
                    if (touchFlags & 0b001)
                    {
                        currReflectTimer = 4;
                        reflectVec.m_y = -1.0f;
                    }
                    if (touchFlags & 0b010)
                    {
                        if (currXSpeed <= -0.5f)
                        {
                            currReflectTimer = 4;
                            reflectVec.m_x = -1.0f;
                        }
                    }
                    else if (touchFlags & 0b100)
                    {
                        if (currXSpeed >= 0.5f)
                        {
                            currReflectTimer = 4;
                            reflectVec.m_x = -1.0f;
                        }
                    }
                    if (currReflectTimer != 0)
                    {
                        moduleAccesser->m_enumerationStart->m_soundModule->playSE(snd_se_common_kick_hit_s, 1, 1, 0);
                        moduleAccesser->m_enumerationStart->m_kineticModule->reflectSpeed(&reflectVec, &momentumReflectAttr);
                    }
                }
            }

            currReflectTimer = (currReflectTimer > 0) ? currReflectTimer - 1 : 0;
            reflectLockoutTimer[fighterPlayerNo] = currReflectTimer;
        }
    }

    u32 transitionOverrideCallback(Fighter* fighterIn, int transitionTermIDIn, u32 targetActionIn)
    {
        u32 result = targetActionIn;
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;
            u32 currStatus = statusModule->getStatusKind();
            int lastTransitionUnitID = statusModule->m_transitionModule->getLastTransitionInfo()->m_unitId;

            int promptedByButton = 0;
            switch (currStatus)
            {
                case Fighter::Status::Jump_Squat:
                {
                    if (lastTransitionUnitID == Fighter::Status::Transition::Term_Cont_Jump_Squat_Button)
                    {
                        promptedByButton = 1;
                    }
                    break;
                }
                case Fighter::Status::Cliff_Jump1:
                case Fighter::Status::Cliff_Jump2:
                case Fighter::Status::Cliff_Jump3:
                {
                    if (lastTransitionUnitID == Fighter::Status::Transition::Term_Cont_Cliff_Jump_Button)
                    {
                        promptedByButton = 1;
                    }
                    break;
                }
                default:
                {
                    promptedByButton = -1;
                    break;
                }
            }
            if (promptedByButton != -1)
            {
                soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
                bool doCharge = (promptedByButton == 1) ? 
                    controllerModule->getButton().m_mask & ipPadButton::MASK_JUMP :
                    controllerModule->getStickY() >= ftValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Common_Param_Float_Jump_Stick_Y, 0);
                float currCharge = chargeAmount[fighterPlayerNo];
                if (doCharge && (currCharge < maxChargeMul))
                {
                    result = 0xFFFFFFFF;
                    currCharge += chargePerFrame;
                    chargeAmount[fighterPlayerNo] = currCharge;
                    OSReport_N("%sP%d Charge Incremented to %.02f\n", outputTag, fighterPlayerNo, chargeAmount[fighterPlayerNo]);
                }
                else if (currCharge > minChargeMul)
                {
                    u8 framesSinceGuardPress = controllerModule->getTriggerCount(soController::Pad_Button_Guard);
                    u8 framesSinceAttackPress = controllerModule->getTriggerCount(soController::Pad_Button_Attack);
                    if (framesSinceGuardPress != framesSinceAttackPress && framesSinceGuardPress <= 2)
                    {
                        result = Fighter::Status::Escape_Air;
                    }
                }
            }
            else
            {
                switch (transitionTermIDIn)
                {
                    case Fighter::Status::Transition::Term_Cont_Dash:
                    case Fighter::Status::Transition::Term_Cont_Turn_Dash:
                    case Fighter::Status::Transition::Term_Cont_Turn_Dash_Dash:
                    {
                        result = 0xFFFFFFFF;
                        break;
                    }
                }
            }
        }
        return result;
    }

    void onStatusChangeCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (fighterPlayerNo < fighterHooks::maxFighterCount)
        {
            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
            soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesser->m_enumerationStart->m_statusModule;

            u32 currStatus = statusModule->getStatusKind();
            if (currStatus == Fighter::Status::Jump_Squat)
            {
                moduleAccesser->m_enumerationStart->m_kineticModule->clearSpeedAll();
            }
            if (moduleAccesser->m_enumerationStart->m_situationModule->getKind() == Situation_Air)
            {
                statusModule->unableTransitionTerm(Fighter::Status::Transition::Term_Stop_Ceil, 0);
                statusModule->unableTransitionTermGroup(Fighter::Status::Transition::Group_Chk_Air_Wall_Jump);
            }
            else
            {
                chargeAmount[fighterPlayerNo] = minChargeMul;
            }
        }
    }

    struct paramInfoBuffer
    {
        soModuleAccesser* m_moduleAccesser;
        ftValueAccesser::ParamFloat m_paramID;
        double m_currValue;
    };

    double doParamModifications(paramInfoBuffer& paramInfo)
    {
        double result = paramInfo.m_currValue;
        soModuleAccesser* moduleAccesserIn = paramInfo.m_moduleAccesser;
        ftValueAccesser::ParamFloat paramIn = paramInfo.m_paramID;

        if (moduleAccesserIn->m_stageObject->m_taskCategory == gfTask::Category_Fighter)
        {
            Fighter* fighterIn = (Fighter*)moduleAccesserIn->m_stageObject;
            u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
            if (fighterPlayerNo < fighterHooks::maxFighterCount)
            {
                soStatusModule* statusModule = moduleAccesserIn->m_enumerationStart->m_statusModule;
                u32 currStatus = statusModule->getStatusKind();
                switch (paramIn)
                {
                    case ftValueAccesser::Customize_Param_Float_Jump_Speed_Y:
                    case ftValueAccesser::Customize_Param_Float_Mini_Jump_Speed_Y:
                    case ftValueAccesser::Customize_Param_Float_Cliff_Jump_Speed_Y:
                    {
                        float currCharge = chargeAmount[fighterPlayerNo];
                        if (currStatus != Fighter::Status::Jump)
                        {
                            currCharge /= 2.0f;
                        }
                        currCharge += 1.0f;
                        OSReport_N("%sApplied P%d Charge (%.02f)\n", outputTag, fighterPlayerNo, currCharge);
                        result *= currCharge;
                        break;
                    }
                    case ftValueAccesser::Customize_Param_Float_Air_Accel_X_Add:
                    case ftValueAccesser::Customize_Param_Float_Air_Accel_X_Mul:
                    case ftValueAccesser::Customize_Param_Float_Air_Brake_X:
                    {
                        if (currStatus <= Fighter::Status::Test_Motion
                            && currStatus != Fighter::Status::Attack_Air && currStatus != Fighter::Status::Fall_Special)
                        {
                            result = 0.0f;
                        }
                        break;
                    }
                    case ftValueAccesser::Customize_Param_Float_Jump_Aerial_Speed_X_Mul:
                    {
                        result = 1.0f;
                    }
                }
            }
        }

        return result;
    }
    double(*_getConstantFloatCore)(soValueAccesser*, soModuleAccesser*, ftValueAccesser::ParamFloat, u32);
    double getConstantFloatCore(soValueAccesser* valueAccesserIn, soModuleAccesser* moduleAccesserIn, ftValueAccesser::ParamFloat paramIn, u32 param_3)
    {   
        paramInfoBuffer paramInfo;
        paramInfo.m_paramID = paramIn;
        paramInfo.m_moduleAccesser = moduleAccesserIn;
        switch (paramIn)
        {
            case ftValueAccesser::Customize_Param_Float_Walk_Accel_Add:
            {
                paramIn = ftValueAccesser::Customize_Param_Float_Walk_Speed_Max;
            }
            case ftValueAccesser::Customize_Param_Float_Jump_Speed_Y:
            {
                soStatusModuleImpl* statusModule = (soStatusModuleImpl*)moduleAccesserIn->m_enumerationStart->m_statusModule;
                if (statusModule->m_statusKind == Fighter::Status::Jump)
                {
                    paramIn = ftValueAccesser::Customize_Param_Float_Mini_Jump_Speed_Y;
                }
                break;
            }
            case ftValueAccesser::Customize_Param_Float_Jump_Speed_X:
            case ftValueAccesser::Customize_Param_Float_Air_Speed_X_Stable:
            {
                paramIn = ftValueAccesser::Customize_Param_Float_Dash_Speed;
                break;
            }
        }
        paramInfo.m_currValue = _getConstantFloatCore(valueAccesserIn, moduleAccesserIn, paramIn, param_3);
        return doParamModifications(paramInfo);
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_TransitionOverrideCB = (fighterHooks::TransitionTermEventCB)transitionOverrideCallback,
        .m_FighterOnStatusChangeCB = (fighterHooks::FighterOnStatusChangeCB)onStatusChangeCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
        SyringeCompat::syReplaceFuncRel(0x14B0BC, reinterpret_cast<void*>(getConstantFloatCore), reinterpret_cast<void**>(&_getConstantFloatCore), Modules::SORA_MELEE);
    }
}