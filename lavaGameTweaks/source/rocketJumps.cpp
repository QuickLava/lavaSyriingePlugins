#include "rocketJumps.h"
#include <ft/ft_value_accesser.h>

namespace rocketJumps
{
	const char outputTag[] = "[lavaRocketJumps] ";
	const char meterChangeStr[] = "%sFighter[%02X] %s: %+.1f Meter, Total: %d (%.1f)!\n";

	u8 infiniteMeterModeFlags = 0;
	const u32 maxStocks = 0x2;
	const float meterStockSize = 25.0f;
	const fighterMeters::meterConfiguration meterConf = { meterStockSize * maxStocks, meterStockSize };

	const u32 curledVar = 0x2200003B;
	const float chargeMax = 1.0f;
	const float chargeFramesToMax = 30.0f;
	const float chargeRate = chargeMax / chargeFramesToMax;
	const float chargeForFastfall = chargeMax * 0.25f;
	float chargeBuffer[fighterHooks::maxFighterCount] = {};

	soMotionChangeParam curlMotionReq = { Fighter::Motion_Item_Screw_Fall, 0.0f, 2.0f };

	const u32 hitboxID = 0x00;
	const u32 hitboxGroup = 0x00;
	const u32 hitboxDuration = 2;
	const float hitboxKBGBase = 0.0f;
	const float hitboxKBGMax = 50.0f;
	const float hitboxKBGChargeDiff = hitboxKBGMax - hitboxKBGBase;
	const float hitboxDamageBase = 8.0f;
	const float hitboxDamageMax = 12.0f;
	const float hitboxDamageChargeDiff = hitboxDamageMax - hitboxDamageBase;
	soCollisionAttackData baseHitbox;
	u8 hitboxRemainingDurationBuffer[fighterHooks::maxFighterCount] = {};

	void doMeterGain(Fighter* fighterIn, float damage)
	{
		u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
		if (fighterPlayerNo < fighterHooks::maxFighterCount)
		{
			soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
			fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;

			int initialStockCount = targetMeterBundle->getMeterStocks();
			targetMeterBundle->addMeter(damage);
			int finalStockCount = targetMeterBundle->getMeterStocks();
			int changeInStockCount = finalStockCount - initialStockCount;

			if (changeInStockCount > 0)
			{
				mechHub::playSE(fighterIn, (SndID)((snd_se_narration_one + 1) - finalStockCount));
				mechHub::reqCenteredGraphic(fighterIn, ef_ptc_common_cliff_catch, 2.5f, 1);
			}

			OSReport_N(meterChangeStr, outputTag, fighterPlayerNo, "Attack Landed", damage, finalStockCount, targetMeterBundle->getMeterStockRemainder());
		}
	}

	void onFighterCreateCallback(Fighter* fighterIn)
	{
		u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
		if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_ROCKET_JUMPS))
		{
			fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;
			targetMeterBundle->setMeterConfig(meterConf, 1);
		}
	}
	void onHitCallback(Fighter* attacker, StageObject* target, float damage)
	{
		u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(attacker);
		if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_ROCKET_JUMPS))
		{
			doMeterGain(attacker, damage);
		}
	}
	void onUpdateCallback(Fighter* fighterIn)
	{
		u32 fighterPlayerNo = fighterHooks::getFighterPlayerNo(fighterIn);
		if (mechHub::getActiveMechanicEnabled(fighterPlayerNo, mechHub::amid_ROCKET_JUMPS))
		{
			soModuleEnumeration* moduleEnum = fighterIn->m_moduleAccesser->m_enumerationStart;
			fighterMeters::meterBundle* targetMeterBundle = fighterMeters::playerMeters + fighterPlayerNo;

			soStatusModule* statusModule = moduleEnum->m_statusModule;
			u32 currStatus = statusModule->getStatusKind();

			soControllerModule* controllerModule = moduleEnum->m_controllerModule;
			ipPadButton justPressed = controllerModule->getTrigger();
			ipPadButton pressed = controllerModule->getButton();

			bool infiniteMeterMode = (infiniteMeterModeFlags >> fighterPlayerNo) & 0b1;

			u8* remainingDurationPtr = hitboxRemainingDurationBuffer + fighterPlayerNo;
			u32 remainingDuration = *remainingDurationPtr;
			if (remainingDuration > 0)
			{
				remainingDuration--;
				*remainingDurationPtr = remainingDuration;
			}
			else
			{
				moduleEnum->m_collisionAttackModule->clear(hitboxID);
			}

			if (moduleEnum->m_situationModule->getKind() != 0x0
				&& (infiniteMeterMode || targetMeterBundle->getMeterStocks() > 0)
				&& (!mechHub::isAttackingStatusKind(currStatus)))
			{
				soWorkManageModule* workManageModule = moduleEnum->m_workManageModule;
				bool curled = workManageModule->isFlag(curledVar);

				float* chargeAmountPtr = chargeBuffer + fighterPlayerNo;

				if (!curled && (justPressed.m_mask & mechHub::allTauntPadMask) != 0x00)
				{
					workManageModule->onFlag(curledVar);
					soMotionModule* motionModule = moduleEnum->m_motionModule;
					motionModule->changeMotion(&curlMotionReq);
					motionModule->setLoopFlag(1);
					*chargeAmountPtr = 0.0f;
				}
				else if (curled)
				{
					float chargeAmount = *chargeAmountPtr;
					chargeAmount += chargeRate;
					*chargeAmountPtr = chargeAmount;

					if (chargeAmount >= chargeMax 
						|| ((pressed.m_mask & mechHub::allTauntPadMask) == 0x00))
					{
						targetMeterBundle->addMeterStocks(-1);

						mechHub::playSE(fighterIn, snd_se_item_Clacker_exp);
						mechHub::reqCenteredGraphic(fighterIn, ef_ptc_common_bomb_a, 1.0f, 0);
						mechHub::reqCenteredGraphic(fighterIn, ef_ptc_common_clacker_bomb, 1.0f, 0);

						soKineticModule* kineticModule = moduleEnum->m_kineticModule;
						soKineticEnergy* gravityEnergy = kineticModule->getEnergy(Fighter::Kinetic_Energy_Gravity);
						gravityEnergy->clearSpeed();

						u32 targetStatus;
						soModuleAccesser* moduleAccesser = fighterIn->m_moduleAccesser;
						float yBoostValue = 0.0f;
						if (controllerModule->getStickY() >= 0.0f)
						{
							targetStatus = Fighter::Status_Item_Screw_Fall;
							yBoostValue = soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Jump_Speed_Y, 0);
							yBoostValue *= (1.0f + (chargeAmount / 2.0f));
						}
						else
						{
							targetStatus = Fighter::Status_Pass;
							u32 targetAttr = ftValueAccesser::Customize_Param_Float_Air_Speed_Y_Stable;
							float fallSpeed = soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Air_Speed_Y_Stable, 0);
							float fastFallSpeed = soValueAccesser::getConstantFloat(moduleAccesser, ftValueAccesser::Customize_Param_Float_Dive_Speed_Y, 0);
							yBoostValue = (chargeAmount < chargeForFastfall) ? fallSpeed : fastFallSpeed;
							yBoostValue *= -1.0f;
						}

						statusModule->changeStatus(targetStatus, moduleAccesser);
						Vec3f boostVec = { 0.0f, yBoostValue, 0.0f };
						kineticModule->addSpeed(&boostVec, moduleAccesser);

						soCollisionAttackData* blastHitboxPtr = &baseHitbox;
						blastHitboxPtr->m_power = (hitboxDamageChargeDiff * chargeAmount) + hitboxDamageBase;
						blastHitboxPtr->m_reactionEffect = (hitboxKBGChargeDiff * chargeAmount) + hitboxKBGBase;
						moduleEnum->m_collisionAttackModule->set(hitboxID, hitboxGroup, blastHitboxPtr);
						*remainingDurationPtr = hitboxDuration;

						OSReport_N("%sRocket Jump! Charge: %0.2f, Vel: %0.2f\n", outputTag, chargeAmount, yBoostValue);
					}
				}
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
		mechHub::initDefaultHitboxData(&baseHitbox);
		baseHitbox.m_vector = 84;
		baseHitbox.m_reactionAdd = 110;
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