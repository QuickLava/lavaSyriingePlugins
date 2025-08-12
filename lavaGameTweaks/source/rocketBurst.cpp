#include "rocketBurst.h"
#include <ft/ft_value_accesser.h>

namespace rocketBurst
{
    char outputTag[] = "[rocketBurst] ";
    char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

    u8 infiniteMeterModeFlags = 0;
    const u32 maxStocks = 0x2;
    const float meterStockSize = 50.0f;
    const fighterMeters::meterConfiguration meterConf = { meterStockSize * maxStocks, meterStockSize };

    const u32 curledVar = 0x2200003B;
    const float chargeMax = 1.0f;
    const float chargeFramesToMax = 30.0f;
    const float chargeRate = chargeMax / chargeFramesToMax;
    float chargeArr[fighterHooks::maxFighterCount] = {};

    Vec3f curlKnockbackMul = { 0.5f, 0.5f, 0.0f };

    const u32 hitboxID = 0x00;
    const u32 hitboxGroup = 0x00;
    const u32 hitboxDuration = 4;
    const float hitboxKBGBase = 50.0f;
    const float hitboxKBGMax = 70.0f;
    const float hitboxKBGChargeDiff = hitboxKBGMax - hitboxKBGBase;
    const float hitboxDamageBase = 8.0f;
    const float hitboxDamageMax = 12.0f;
    const float hitboxDamageChargeDiff = hitboxDamageMax - hitboxDamageBase;
    const float hitboxSizeBase = 10.0f;
    const float hitboxSizeMax = 12.5f;
    const float hitboxSizeChargeDiff = hitboxSizeMax - hitboxSizeBase;
    const float hitboxGFXSizeBase = 1.0f;
    const float hitboxGFXSizeMax = 1.25f;
    const float hitboxGFXSizeChargeDiff = hitboxGFXSizeMax - hitboxGFXSizeBase;
    u8 framesSinceBurstArr[fighterHooks::maxFighterCount] = { };

    void onFighterCreateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_ROCKET_BURST))
        {
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            targetMeterBundle->setMeterConfig(meterConf, 1);
        }
    }
    void onAttackCallback(Fighter* attacker, StageObject* target, float damage, StageObject* projectile, u32 attackKind, u32 attackSituation)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_ROCKET_BURST))
        {
            mechUtil::doMeterGain(attacker, damage, ef_ptc_common_hit_normal_b, 1.0f, mechUtil::mgac_ON_STOCK_GAIN);
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            OSReport_N(meterChangeStr, outputTag, fighterPlayerNo, "Attack Landed",
                damage, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
        }
    }
    void onUpdateCallback(Fighter* fighterIn)
    {
        u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
        if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_ROCKET_BURST))
        {
            fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
            if (mechHub::getActiveMechanicEnabledDiff(fighterPlayerNo, mechHub::amid_AIRDODGE_CANCELS))
            {
                targetMeterBundle->setMeterConfig(meterConf, 1);
                OSReport_N("%sMeter Reset Handled\n", outputTag);
            }

            soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;

            soStatusModule* statusModule = moduleAccesser->m_enumerationStart->m_statusModule;
            u32 currStatus = statusModule->getStatusKind();

            soControllerModule* controllerModule = moduleAccesser->m_enumerationStart->m_controllerModule;
            ipPadButton justPressed = controllerModule->getTrigger();
            ipPadButton pressed = controllerModule->getButton();

            u32 framesSinceBurst = framesSinceBurstArr[fighterPlayerNo];
            if (framesSinceBurst < 0xFF)
            {
                framesSinceBurstArr[fighterPlayerNo] = ++framesSinceBurst;
            }
            if (framesSinceBurst == 0x01 && (currStatus == Fighter::Status_Item_Screw_Fall || currStatus == Fighter::Status_Pass))
            {
                float chargeAmount = chargeArr[fighterPlayerNo];
                float graphicSize = (chargeAmount * hitboxGFXSizeChargeDiff) + hitboxGFXSizeBase;
                mechUtil::playSE(fighterIn, snd_se_item_Clacker_exp);
                mechUtil::reqCenteredGraphic(fighterIn, ef_ptc_common_bomb_a, graphicSize, 0);
                mechUtil::reqCenteredGraphic(fighterIn, ef_ptc_common_clacker_bomb, graphicSize, 0);
            }
            else if (framesSinceBurst == hitboxDuration)
            {
                moduleAccesser->getCollisionAttackModule().clear(hitboxID);
            }

            u32 currStatusCurlCost = 0xFFFFFFFF;
            soWorkManageModule* workManageModule = moduleAccesser->m_enumerationStart->m_workManageModule;
            bool curled = workManageModule->isFlag(curledVar);
            u32 currMeterStocks = targetMeterBundle->getMeterStocks();
            if (moduleAccesser->getSituationModule().getKind() == 0x00)
            {
                chargeArr[fighterPlayerNo] = 0.0f;
            }
            else
            {
                if (curled || statusModule->isEnableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Attack))
                {
                    currStatusCurlCost = 0x0;
                }
                else if (currStatus == Fighter::Status_Fall_Special || currStatus == Fighter::Status_Damage_Fly || currStatus == Fighter::Status_Damage_Fly_Roll)
                {
                    currStatusCurlCost = 0x1;
                }
            }
            bool infiniteMeterMode = (infiniteMeterModeFlags >> fighterPlayerNo) & 0b1;
            if (infiniteMeterMode)
            {
                currMeterStocks = maxStocks;
            }
            if (currMeterStocks > currStatusCurlCost)
            {
                float chargeAmount = chargeArr[fighterPlayerNo];
                if (!curled && (justPressed.m_mask & mechUtil::allTauntPadMask) != 0x00)
                {
                    targetMeterBundle->addMeterStocks(-currStatusCurlCost);
                    OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterPlayerNo(fighterIn), "Curl Init",
                        -meterStockSize * currStatusCurlCost, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());

                    workManageModule->setInt(0x1, Fighter::Instance_Work_Int_No_Tread_Frame);
                    statusModule->changeStatus(Fighter::Status_Item_Screw_Fall, moduleAccesser);
                    statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Attack);
                    statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Special);
                    statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Escape);
                    statusModule->unableTransitionTermGroup(Fighter::Status_Transition_Term_Group_Chk_Air_Jump_Aerial);
                    workManageModule->onFlag(curledVar);
                    chargeAmount = 0.0f;
                }
                else if (curled)
                {
                    chargeAmount += chargeRate;

                    if (chargeAmount >= chargeMax 
                        || ((pressed.m_mask & mechUtil::allTauntPadMask) == 0x00))
                    {
                        soKineticModule* kineticModule = moduleAccesser->m_enumerationStart->m_kineticModule;
                        kineticModule->getEnergy(Fighter::Kinetic_Energy_Gravity)->clearSpeed();
                        kineticModule->getEnergy(Fighter::Kinetic_Energy_Damage)->mulSpeed(&curlKnockbackMul);

                        u32 targetStatus;
                        float yBoostValue = 0.0f;
                        if (controllerModule->getStickY() >= 0.0f)
                        {
                            targetStatus = Fighter::Status_Item_Screw_Fall;
                            yBoostValue = soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Jump_Speed_Y, 0);
                            float airJumpMult = soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Jump_Aerial_Speed_Y, 0);
                            airJumpMult = MAX(airJumpMult, 0.75f);
                            yBoostValue *= (chargeAmount * 0.5f) + airJumpMult;
                        }
                        else
                        {
                            targetStatus = Fighter::Status_Pass;
                            yBoostValue = -soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Dive_Speed_Y, 0);
                        }
                        float xBoostValue = soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Jump_Aerial_Speed_X_Mul, 0);
                        xBoostValue = (chargeAmount * 0.5f) + xBoostValue;
                        xBoostValue *= soValueAccesser::getVariableFloat(moduleAccesser, ftValueAccesser::Var_Float_Controller_Stick_X_Lr, 0);

                        statusModule->changeStatus(targetStatus, moduleAccesser);
                        Vec3f boostVec = { xBoostValue, yBoostValue, 0.0f };
                        workManageModule->onFlag(Fighter::Instance_Work_Flag_No_Speed_Operation_Chk);
                        if (!mechUtil::isDamageStatusKind(statusModule->getPrevStatusKind(0)))
                        {
                            kineticModule->clearSpeedAll();
                        }
                        kineticModule->addSpeed(&boostVec, moduleAccesser);

                        soCollisionAttackData blastHitbox;
                        mechUtil::initDefaultHitboxData(&blastHitbox);
                        blastHitbox.m_vector = 84;
                        blastHitbox.m_reactionAdd = 90;
                        blastHitbox.m_hitStopFrame = 1.5f;
                        blastHitbox.m_attribute = soCollisionAttackData::Attribute_Fire;
                        blastHitbox.m_power = (hitboxDamageChargeDiff * chargeAmount) + hitboxDamageBase;
                        blastHitbox.m_reactionEffect = (hitboxKBGChargeDiff * chargeAmount) + hitboxKBGBase;
                        blastHitbox.m_size = (hitboxSizeChargeDiff * chargeAmount) + hitboxSizeBase;
                        moduleAccesser->getCollisionAttackModule().set(hitboxID, hitboxGroup, &blastHitbox);

                        targetMeterBundle->addMeterStocks(-1);
                        OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterPlayerNo(fighterIn), "Rocket Jump",
                            -meterStockSize, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
                        OSReport_N("%sRocket Jump! Charge: %0.2f, Vel: %0.2f, %0.2f\n", outputTag, chargeAmount, xBoostValue, yBoostValue);
                        framesSinceBurstArr[fighterPlayerNo] = 0x00;
                    }
                }

                chargeArr[fighterPlayerNo] = chargeAmount;
            }

            if (pressed.m_attack && pressed.m_special && pressed.m_jump && (justPressed.m_mask & mechUtil::allTauntPadMask))
            {
                infiniteMeterModeFlags ^= (1 << fighterPlayerNo);
                targetMeterBundle->resetMeter();
                OSReport_N("%sInfinite Meter Mode Status: %d!\n", outputTag, !infiniteMeterMode);
            }
        }
    }

    void onMeleeStartCallback()
    {
        infiniteMeterModeFlags = 0;

        u32* framesSinceArrPtr = (u32*)framesSinceBurstArr;
        framesSinceArrPtr[0x00] = 0xFFFFFFFF;
        framesSinceArrPtr[0x01] = 0xFFFFFFFF;
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_MeleeOnStartCB = (fighterHooks::MeleeOnStartCB)onMeleeStartCallback,
        .m_FighterOnCreateCB = (fighterHooks::FighterOnCreateCB)onFighterCreateCallback,
        .m_FighterOnUpdateCB = (fighterHooks::FighterOnUpdateCB)onUpdateCallback,
        .m_FighterOnAttackCB = (fighterHooks::FighterOnAttackCB)onAttackCallback,
    };
#pragma c99 off

    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }
}