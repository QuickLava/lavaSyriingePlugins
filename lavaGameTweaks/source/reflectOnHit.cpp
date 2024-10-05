#include "reflectOnHit.h"

namespace reflectOnHit
{
    const char outputTag[] = "[lavaReflectOnHit] ";

	void reflectBoxCB(Fighter* fighterIn)
	{
		typedef bool (*ftGetReflector)(ftOwner*);
		const ftGetReflector getReflectorPtr = (ftGetReflector)0x8081C8A4;
		typedef int* (*vecAt)(void*, int);
		typedef void (*setStatusAll)(void*, int, int);

		ftOwner* ownerPtr = fighterIn->getOwner();
		if (getReflectorPtr(ownerPtr) == 0)
		{
			soCollisionAttackModuleImpl* attackModule = (soCollisionAttackModuleImpl*)fighterIn->m_moduleAccesser->m_enumerationStart->m_collisionAttackModule;
			void* attackPartArrPtr = attackModule->m_collisionAttackPart;
			int* vTable = ((int**)attackPartArrPtr)[0];
			vecAt atFuncPtr = (vecAt)(vTable[0x3]);

			int* firstEntry = atFuncPtr(attackPartArrPtr, 0);
			if (firstEntry != NULL)
			{
				bool hitboxOut = *firstEntry != 0;

				void* reflectorModule = fighterIn->m_moduleAccesser->m_enumerationStart->m_collisionReflectorModule;
				int* vTable = *(int**)reflectorModule;
				setStatusAll setStatusAllPtr = (setStatusAll)(vTable[0x10]);
				setStatusAllPtr(reflectorModule, hitboxOut, 1);
			}
		}
	}
	void reflectTargetOnHitCallback(Fighter* attacker, StageObject* target, float damage)
	{
		typedef void (*itReflect)(StageObject*);
		typedef void (*wnReflect)(StageObject*);

		if (target != NULL)
		{
			int targetCategory = fighterHooks::getCatPtr(target);
			soSoundModule* soundModule = target->m_moduleAccesser->getSoundModule();
			if (targetCategory == 0xB)
			{
				itReflect reflectItem = (itReflect)0x80995EE0;
				reflectItem(target);
				soundModule->playSE(snd_se_narration_three, 1, 1, 0);
			}
			else if (targetCategory == 0xC)
			{
				int* weaponVTable9 = ((int**)target)[0xF] + 0x49;
				wnReflect funcPtr = (wnReflect)weaponVTable9[0x42];
				funcPtr(target);
				soundModule->playSE(snd_se_narration_four, 1, 1, 0);
			}
		}
	}
	// Returns 1 if clank is allowed, 0 otherwise!
	bool validateClankBody(StageObject* attackerPtr, StageObject* targetPtr)
	{
		const char fmtStr[] = "%s- %s (Cat: 0x%04X)\n";

		bool result = 1;

		int attackerCategory = fighterHooks::getCatPtr((gfTask*)attackerPtr);
		int targetCategory = fighterHooks::getCatPtr((gfTask*)targetPtr);
		OSReport_N("%sChecking Collision Between:\n", outputTag);
		OSReport_N(fmtStr, outputTag, attackerPtr->m_taskName, attackerCategory);
		OSReport_N(fmtStr, outputTag, targetPtr->m_taskName, targetCategory);
		if (attackerCategory == 0xA && (targetCategory == 0xB || targetCategory == 0xC))
		{
			OSReport_N("%s- Clank Disabled!\n", outputTag);
			reflectTargetOnHitCallback((Fighter*)attackerPtr, targetPtr, 0.0f);
			result = 0;
		}
		return result;
	}
	asm void disableArticleAndItemClankHook()
	{
		nofralloc
			mflr r31;			  // Backup LR in a non-volatile register!
			                      // Call main function body!
			lwz r3, 0x8(r4);	  // param1 is Attacker StageObject*
			mr r4, r5;			  // param2 is Target StageObject*
			bl validateClankBody; // Call!
			
			mtlr r31;			  // Restore Trampoline LR!
			cmplwi r3, 0x01;	  // Compare the return value to 1...
			beq skipLRHack;		  // ... and if it's 1 we'll continue our function like normal!
								  // Otherwise, we're gonna return from the function early!
			lmw r4, 0x0C(r1);	  // Restore all but r3, so we can pass back our return value!
			lwz r1, 0x00(r1);     // Deallocate Trampoline stack frame!
			lwz r1, 0x00(r1);     // Deallocate Pre-Trampoline stack frame!
			lwz r0, 0x04(r1);     // Grab Pre-Trampoline LR...
			mtlr r0;              // ... and write it to LR so we return there instead!
		skipLRHack:
			blr; 				  // Return, either to Trampoline or to Pre-Trampoline!
	}

	StageObject* getStageObjFromCollLog(soCollisionLog* collisionLog)
	{
		typedef StageObject* (*getTaskByIDPtr)(void*, u32, int);
		void** const g_gfTaskSchedulerPtrAddr = (void**)0x805A0068;
		const getTaskByIDPtr getTaskByID = (getTaskByIDPtr)0x8002F018;

		return (StageObject*)getTaskByID(*g_gfTaskSchedulerPtrAddr, collisionLog->m_category, collisionLog->m_taskId);
	}
	// Returns ID of replacement GFX!
	void reflectEffects(StageObject* attackerPtr, soCollisionLog* collisionLog)
	{
		StageObject* targetPtr = getStageObjFromCollLog(collisionLog);

		int attackerCategory = fighterHooks::getCatPtr((gfTask*)attackerPtr);
		int targetCategory = fighterHooks::getCatPtr((gfTask*)targetPtr);
		if (attackerCategory == 0xA && (targetCategory == 0xB || targetCategory == 0xC))
		{
			OSReport_N("%sDoing Reflect Effects:\n", outputTag);

			soSoundModule* soundModule = attackerPtr->m_moduleAccesser->getSoundModule();
			soundModule->playSE(snd_se_item_Item_Get, 1, 1, 0);
		}
	}
	asm void reflectEffectsOnArticleAndItemClankHook()
	{
		nofralloc
		mflr r31;			  // Backup LR in a non-volatile register!
							  // Call main function body!
		lwz r3, 0x44(r3);	  // param1 is Attacker StageObject*; grab moduleAccesser from effectModule...
		lwz r3, 0x08(r3);	  // ... and StageObject* from there.
							  // param2 is collisionLog (already in r4)
		bl reflectEffects;	  // Call!

		mtlr r31;			  // Restore LR...
		blr; 				  // ... and return!
	}

	int preserveProjectiles(StageObject* objectIn, int isInflictMaskIn)
	{
		int result = isInflictMaskIn;

		int objectCategory = fighterHooks::getCatPtr((gfTask*)objectIn);;
		if (objectCategory == 0xB || objectCategory == 0xC)
		{
			result &= ~0b1;
		}

		return result;
	}
	asm void disableClangKillingProjectilesHook()
	{
		nofralloc
		mflr r31;			      // Backup LR in a non-volatile register!
							      // Call main function body!
		lwz r3, 0x28(r28);	      // param1 is Attacker StageObject*; grab moduleAccesser from attackModule...
		lwz r3, 0x08(r3);	      // ... and StageObject* from there.
							      // param2 is isInflictMask (already in r4)
		bl preserveProjectiles;	  // Call!

		stw r3, 0xC(r1);		  // Write processed result over r4 backup on stack, to replace isInflictMask!

		mtlr r31;			      // Restore LR...
		blr; 				      // ... and return!
	}

	void registerHooks()
	{
		fighterHooks::ftCallbackMgr::registerOnUpdateCallback(reflectBoxCB);
		fighterHooks::ftCallbackMgr::registerOnAttackCallback(reflectTargetOnHitCallback);
		// Disable Fighter vs Item/Article Clanks @ 0x8074651C: 0x0C bytes into symbol "checkRebound/[soCollisionAttackModuleImpl]/so_collision_a"
		SyringeCore::syInlineHookRel(0x3BB08, reinterpret_cast<void*>(disableArticleAndItemClankHook), Modules::SORA_MELEE);
		// Effects on Fighter vs Item/Article Clanks @ 0x807A92AC: 0x28 bytes into symbol "notifyEventCollisionAttack/[soEffectModuleImpl]/so_effect"
		SyringeCore::syInlineHookRel(0x9E898, reinterpret_cast<void*>(reflectEffectsOnArticleAndItemClankHook), Modules::SORA_MELEE);
		// Prevent Item/Article Death @ 0x807463E8: 0x1D8 bytes into symbol "check/[soCollisionAttackModuleImpl]/so_collision_attack_m"
		SyringeCore::syInlineHookRel(0x3B9D4, reinterpret_cast<void*>(disableClangKillingProjectilesHook), Modules::SORA_MELEE);
	}
}