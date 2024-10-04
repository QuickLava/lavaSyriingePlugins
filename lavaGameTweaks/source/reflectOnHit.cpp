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

	void registerHooks()
	{
		fighterHooks::ftCallbackMgr::registerOnUpdateCallback(reflectBoxCB);
		fighterHooks::ftCallbackMgr::registerOnAttackCallback(reflectTargetOnHitCallback);
	}
}