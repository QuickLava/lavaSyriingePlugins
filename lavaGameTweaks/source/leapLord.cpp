#include "leapLord.h"

namespace leapLord
{
    char outputTag[] = "[leapLord] ";

    float chargeAmount[fighterHooks::maxFighterCount];
    const float minChargeMul = 0.25f;
    const float maxChargeMul = 1.50f;
    const float maxChargeLen = 40.0f;
    const float chargePerFrame = (maxChargeMul - minChargeMul) / maxChargeLen;

    soInstanceAttribute momentumReflectAttr = 1;

    const u16 chargeableStatuses[] = 
    { 
        Fighter::Status::Jump_Squat, Fighter::Status::Tread_Jump, 
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
            else if (moduleAccesser->m_enumerationStart->m_situationModule->getKind() == Situation_Air)
            {
                soGroundModule* groundModule = moduleAccesser->m_enumerationStart->m_groundModule;
                if (currReflectTimer == 0)
                {
                    Vec3f reflectVec(0.0f, 0.0f, 0.0f);
                    if (groundModule->isTouch(0b0110, 0))
                    {
                        reflectVec.m_x = -1.0f;
                        currReflectTimer = 4;
                    }
                    else if (groundModule->isTouch(0b0001, 0))
                    {
                        reflectVec.m_y = -1.0f;
                        currReflectTimer = 4;
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
            if (moduleAccesser->m_enumerationStart->m_situationModule->getKind() == Situation_Air)
            {
                statusModule->unableTransitionTerm(Fighter::Status::Transition::Term_Stop_Ceil, 0);
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
                float currCharge = chargeAmount[fighterPlayerNo];

                soStatusModule* statusModule = moduleAccesserIn->m_enumerationStart->m_statusModule;
                soSituationModule* situationModule = moduleAccesserIn->m_enumerationStart->m_situationModule;

                u32 currStatus = statusModule->getStatusKind();
                u32 currSituation = situationModule->getKind();

                switch (paramIn)
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
                        result *= currCharge;
                        break;
                    }
                    case ftValueAccesser::Customize_Param_Float_Air_Accel_X_Add:
                    case ftValueAccesser::Customize_Param_Float_Air_Accel_X_Mul:
                    case ftValueAccesser::Customize_Param_Float_Air_Brake_X:
                    {
                        if (currStatus != Fighter::Status::Attack_Air && currStatus <= Fighter::Status::Test_Motion)
                        {
                            result = 0.0f;
                        }
                        break;
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
        switch (paramIn)
        {
            case ftValueAccesser::Customize_Param_Float_Mini_Jump_Speed_Y:
            {
                paramIn = ftValueAccesser::Customize_Param_Float_Jump_Speed_Y;
                break;
            }
            case ftValueAccesser::Customize_Param_Float_Tread_Mini_Jump_Speed_Y_Mul:
            {
                paramIn = ftValueAccesser::Customize_Param_Float_Tread_Jump_Speed_Y_Mul;
                break;
            }
            case ftValueAccesser::Customize_Param_Float_Jump_Speed_X:
            case ftValueAccesser::Customize_Param_Float_Air_Speed_X_Stable:
            {
                paramIn = ftValueAccesser::Customize_Param_Float_Jump_Speed_X_Max;
                break;
            }
        }
        paramInfo.m_paramID = paramIn;
        paramInfo.m_moduleAccesser = moduleAccesserIn;
        paramInfo .m_currValue = _getConstantFloatCore(valueAccesserIn, moduleAccesserIn, paramIn, param_3);
        return doParamModifications(paramInfo);
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
        SyringeCompat::syReplaceFuncRel(0x14B0BC, reinterpret_cast<void*>(getConstantFloatCore), reinterpret_cast<void**>(&_getConstantFloatCore), Modules::SORA_MELEE);
    }
}