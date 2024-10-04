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
		if ((attackerCategory == 0xA && (targetCategory == 0xB || targetCategory == 0xC)) ||
			((attackerCategory == 0xB || attackerCategory == 0xC) && targetCategory == 0xA))
		{
			OSReport_N("%s- Clank Disabled!\n", outputTag);
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

	void registerHooks()
	{
		fighterHooks::ftCallbackMgr::registerOnUpdateCallback(reflectBoxCB);
		fighterHooks::ftCallbackMgr::registerOnAttackCallback(reflectTargetOnHitCallback);
		// Disable Fighter vs Item/Article Clanks @ 0x8074651C: 0x0C bytes into symbol "checkRebound/[soCollisionAttackModuleImpl]/so_collision_a"
		SyringeCore::syInlineHookRel(0x3BB08, reinterpret_cast<void*>(disableArticleAndItemClankHook), Modules::SORA_MELEE);
	}
}