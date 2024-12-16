#include "rocketBurst.h"
#include <ft/ft_value_accesser.h>

namespace rocketBurst
{
	char outputTag[] = "[lavaRocketBurst] ";
	char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

	u8 infiniteMeterModeFlags = 0;
	const u32 maxStocks = 0x3;
	const float meterStockSize = 25.0f;
	const fighterMeters::meterConfiguration meterConf = { meterStockSize * maxStocks, meterStockSize };

	const u32 curledVar = 0x2200003B;
	const float chargeMax = 1.0f;
	const float chargeFramesToMax = 30.0f;
	const float chargeRate = chargeMax / chargeFramesToMax;
	float chargeArr[fighterHooks::maxFighterCount] = {};

	soMotionChangeParam curlMotionReq = { Fighter::Motion_Item_Screw_Fall, 0.0f, 2.0f };
	Vec3f curlKnockbackMul = { 0.5f, 0.5f, 0.0f };

	const u32 hitboxID = 0x00;
	const u32 hitboxGroup = 0x00;
	const u32 hitboxDuration = 3;
	const float hitboxKBGBase = 25.0f;
	const float hitboxKBGMax = 50.0f;
	const float hitboxKBGChargeDiff = hitboxKBGMax - hitboxKBGBase;
	const float hitboxDamageBase = 10.0f;
	const float hitboxDamageMax = 16.0f;
	const float hitboxDamageChargeDiff = hitboxDamageMax - hitboxDamageBase;
	soCollisionAttackData baseHitbox;
	u8 framesSinceJumpArr[fighterHooks::maxFighterCount] = { };

	void onFighterCreateCallback(Fighter* fighterIn)
	{
		u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
		if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_ROCKET_BURST))
		{
			fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
			targetMeterBundle->setMeterConfig(meterConf, 1);
		}
	}
	void onHitCallback(Fighter* attacker, StageObject* target, float damage)
	{
		u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
		if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_ROCKET_BURST))
		{
			mechHub::doMeterGain(attacker, damage, ef_ptc_common_cliff_catch, 2.5f, mechHub::announcerOnStockGain);
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
			soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
			fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;

			soStatusModule* statusModule = moduleEnum->m_statusModule;
			u32 currStatus = statusModule->getStatusKind();

			soControllerModule* controllerModule = moduleEnum->m_controllerModule;
			ipPadButton justPressed = controllerModule->getTrigger();
			ipPadButton pressed = controllerModule->getButton();

			bool infiniteMeterMode = (infiniteMeterModeFlags >> fighterPlayerNo) & 0b1;

			if (moduleEnum->m_situationModule->getKind() == 0x00)
			{
				chargeArr[fighterPlayerNo] = 0.0f;
			}

			u32 framesSinceJump = framesSinceJumpArr[fighterPlayerNo];
			if (framesSinceJump < 0xFF)
			{
				framesSinceJumpArr[fighterPlayerNo] = ++framesSinceJump;
			}
			if (framesSinceJump == 0x01 && (currStatus == Fighter::Status_Item_Screw_Fall || currStatus == Fighter::Status_Pass))
			{
				mechHub::playSE(fighterIn, snd_se_item_Clacker_exp);
				mechHub::reqCenteredGraphic(fighterIn, ef_ptc_common_bomb_a, 1.0f, 0);
				mechHub::reqCenteredGraphic(fighterIn, ef_ptc_common_clacker_bomb, 1.0f, 0);
			}
			else if (framesSinceJump == hitboxDuration)
			{
				moduleEnum->m_collisionAttackModule->clear(hitboxID);
			}

			u32 currStatusCurlCost = 0xFFFFFFFF;
			soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
			bool curled = workManageModule->isFlag(curledVar);
			if (curled
				|| (currStatus >= Fighter::Status_Jump && currStatus <= Fighter::Status_Fall_Aerial)
				|| (currStatus == Fighter::Status_Item_Screw_Fall || currStatus == Fighter::Status_Pass)
				|| currStatus == Fighter::Status_Damage_Fall)
			{
				currStatusCurlCost = 0x0;
			}
			else if (currStatus == Fighter::Status_Fall_Special || currStatus == Fighter::Status_Escape_Air)
			{
				currStatusCurlCost = 0x1;
			}
			else if (currStatus == Fighter::Status_Damage_Fly || currStatus == Fighter::Status_Damage_Fly_Roll)
			{
				currStatusCurlCost = 0x2;
			}

			if (infiniteMeterMode || (targetMeterBundle->getMeterStocks() > currStatusCurlCost))
			{
				float chargeAmount = chargeArr[fighterPlayerNo];
				if (!curled && (justPressed.m_mask & mechHub::allTauntPadMask) != 0x00)
				{
					workManageModule->onFlag(curledVar);
					targetMeterBundle->addMeterStocks(-currStatusCurlCost);
					OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterPlayerNo(fighterIn), "Curl Init",
						-meterStockSize * currStatusCurlCost, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());

					soMotionModule* motionModule = moduleEnum->m_motionModule;
					motionModule->changeMotion(&curlMotionReq);
					motionModule->setLoopFlag(1);
					chargeAmount = 0.0f;
				}
				else if (curled)
				{
					chargeAmount += chargeRate;

					if (chargeAmount >= chargeMax 
						|| ((pressed.m_mask & mechHub::allTauntPadMask) == 0x00))
					{
						soKineticModule* kineticModule = moduleEnum->m_kineticModule;
						kineticModule->getEnergy(Fighter::Kinetic_Energy_Gravity)->clearSpeed();
						kineticModule->getEnergy(Fighter::Kinetic_Energy_Damage)->mulSpeed(&curlKnockbackMul);

						u32 targetStatus;
						soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
						float yBoostValue = 0.0f;
						if (controllerModule->getStickY() >= 0.0f)
						{
							targetStatus = Fighter::Status_Item_Screw_Fall;
							yBoostValue = soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Jump_Speed_Y, 0);
							float airJumpMult = soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Jump_Aerial_Speed_Y, 0);
							yBoostValue *= (chargeAmount * 0.5f) + airJumpMult;
						}
						else
						{
							targetStatus = Fighter::Status_Pass;
							yBoostValue = -soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Dive_Speed_Y, 0);
						}

						statusModule->changeStatus(targetStatus, moduleAccesser);
						Vec3f boostVec = { 0.0f, yBoostValue, 0.0f };
						workManageModule->onFlag(Fighter::Instance_Work_Flag_No_Speed_Operation_Chk);
						kineticModule->addSpeed(&boostVec, moduleAccesser);

						soCollisionAttackData* blastHitboxPtr = &baseHitbox;
						blastHitboxPtr->m_power = (hitboxDamageChargeDiff * chargeAmount) + hitboxDamageBase;
						blastHitboxPtr->m_reactionEffect = (hitboxKBGChargeDiff * chargeAmount) + hitboxKBGBase;
						moduleEnum->m_collisionAttackModule->set(hitboxID, hitboxGroup, blastHitboxPtr);

						targetMeterBundle->addMeterStocks(-1);
						OSReport_N(meterChangeStr, outputTag, fighterHooks::getFighterPlayerNo(fighterIn), "Rocket Jump",
							-meterStockSize, targetMeterBundle->getMeterStocks(), targetMeterBundle->getMeterStockRemainder());
						OSReport_N("%sRocket Jump! Charge: %0.2f, Vel: %0.2f\n", outputTag, chargeAmount, yBoostValue);
						framesSinceJumpArr[fighterPlayerNo] = 0x00;
					}
				}

				chargeArr[fighterPlayerNo] = chargeAmount;
			}

			if (pressed.m_attack && pressed.m_special && pressed.m_jump && (justPressed.m_mask & mechHub::allTauntPadMask))
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

		u32* framesSinceArrPtr = (u32*)framesSinceJumpArr;
		framesSinceArrPtr[0x00] = 0xFFFFFFFF;
		framesSinceArrPtr[0x01] = 0xFFFFFFFF;

		mechHub::initDefaultHitboxData(&baseHitbox);
		baseHitbox.m_vector = 88;
		baseHitbox.m_reactionAdd = 90;
		baseHitbox.m_size = 10.0f;
		baseHitbox.m_attribute = soCollisionAttackData::Attribute_Fire;
	}

	void registerHooks()
	{
		fighterHooks::ftCallbackMgr::registerMeleeOnStartCallback(onMeleeStartCallback);
		fighterHooks::ftCallbackMgr::registerOnAttackCallback(onHitCallback);
		fighterHooks::ftCallbackMgr::registerOnAttackItemCallback((fighterHooks::FighterOnAttackItemCB)onHitCallback);
		fighterHooks::ftCallbackMgr::registerOnAttackArticleCallback((fighterHooks::FighterOnAttackArticleCB)onHitCallback);
		fighterHooks::ftCallbackMgr::registerOnCreateCallback(onFighterCreateCallback);
		fighterHooks::ftCallbackMgr::registerOnUpdateCallback(onUpdateCallback);
	}
}