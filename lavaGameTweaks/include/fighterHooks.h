#ifndef LAVA_FIGHTER_HOOKS_H_V1
#define LAVA_FIGHTER_HOOKS_H_V1

#include <sy_core.h>
#include <string.h>
#include <vector.h>
#include <modules.h>
#include <ft/fighter.h>
#include <ft/ft_manager.h>
#include <gm/gm_global.h>
#include <wn/weapon.h>
#include <it/item.h>

#include <so/collision/so_collision_attack_module_impl.h>

#include "logUtils.h"

namespace fighterHooks
{
	extern const char outputTag[];
	extern const char observerMessageFmt[];

	const u32 maxFighterCount = 0x8;

	typedef int (*gfTaskGetCategory)(gfTask*);
	const gfTaskGetCategory getCatPtr = (gfTaskGetCategory)0x8098C010;

	typedef void (*GenericArglessCB)();
	typedef GenericArglessCB MeleeOnStartCB;
	typedef GenericArglessCB MeleeOnReadyGoCB;
	typedef GenericArglessCB MeleeOnGameSetCB;
	typedef void (*GenericFighterEventCB)(Fighter*);
	typedef GenericFighterEventCB FighterOnCreateCB;
	typedef GenericFighterEventCB FighterOnStartCB;
	typedef GenericFighterEventCB FighterOnRemoveCB;
	typedef GenericFighterEventCB FighterOnUpdateCB;

	typedef void (*FighterOnAttackGenericCB)(Fighter*, StageObject*, float, StageObject*);
	typedef void (*FighterOnAttackCB)(Fighter*, StageObject*, float);
	typedef void (*FighterOnAttackItemCB)(Fighter*, StageObject*, float, BaseItem*);
	typedef void (*FighterOnAttackArticleCB)(Fighter*, StageObject*, float, Weapon*);

#define INCLUDE_OUTSIDE_OBSERVER false

	u32 getFighterSlotNo(Fighter* fighterIn);
	u32 getFighterPlayerNo(Fighter* fighterIn);

	class ftCallbackMgr
	{
	private:
#if INCLUDE_OUTSIDE_OBSERVER
		class ftEventWatcher : public ftOutsideEventObserver
		{
		public:
			ftEventWatcher();
			virtual void notifyEventAppeal(int entryId, int);
			virtual void notifyEventDead(int entryId, int deadCount, int, int);
			virtual void notifyEventSuicide(int entryId);
			virtual void notifyEventSucceedHit(int entryId, u32 consecutiveHits, float totalDamage);
			virtual void notifyEventKnockout(int entryId);
			virtual void notifyEventBeat(int entryId1, int entryId2);
		};
		static ftEventWatcher m_eventWatcher;
		static void subscribeEventWatcher();
		static void unsubscribeEventWatcher();
#endif

		static Vector<void*> m_onMeleeStartCallbacks;
		static Vector<void*> m_onMeleeReadyGoCallbacks;
		static Vector<void*> m_onMeleeGameSetCallbacks;
		static Vector<void*> m_onCreateCallbacks;
		static Vector<void*> m_onStartCallbacks;
		static Vector<void*> m_onRemoveCallbacks;
		static Vector<void*> m_onUpdateCallbacks;
		static Vector<void*> m_onAttackCallbacks;
		static Vector<void*> m_onAttackItemCallbacks;
		static Vector<void*> m_onAttackArticleCallbacks;

		static bool _registerCallback(Vector<void*>* targetVector, void* callbackIn);
		static bool _unregisterCallback(Vector<void*>* targetVector, void* callbackIn);
		static void _performArglessCallbacks(Vector<void*>* targetVector);
		static void _performFighterEventCallbacks(Vector<void*>* targetVector, u32 entryID);
		template<typename cbType>
		static inline bool registerCallback(Vector<void*>& targetVector, cbType callbackIn)
		{
			return _registerCallback(&targetVector, (void*)callbackIn);
		}
		template<typename cbType>
		static inline bool unregisterCallback(Vector<void*>& targetVector, cbType callbackIn)
		{
			return _unregisterCallback(&targetVector, (void*)callbackIn);
		}
		
		template<typename watcherType>
		static void subscribeWatcherToFighter(soEventObserver<watcherType>& watcherIn, Fighter* fighterIn)
		{
			watcherIn.setupObserver(fighterIn->m_moduleAccesser->getEventManageModule()->getManageId());
			OSReport_N(observerMessageFmt, outputTag, "Subscribed Watcher", watcherIn.m_manageID, watcherIn.m_unitID, watcherIn.m_sendID);
		}
		template<typename watcherType>
		static void unsubscribeWatcher(soEventObserver<watcherType>& watcherIn)
		{
			OSReport_N(observerMessageFmt, outputTag, "Unsubscribing Watcher", watcherIn.m_manageID, watcherIn.m_unitID, watcherIn.m_sendID);
			watcherIn.removeObserver();
		}

	public:
		// OnMeleeStart Callbacks
		static bool registerMeleeOnStartCallback(MeleeOnStartCB callbackIn);
		static bool unregisterMeleeOnStartCallback(MeleeOnStartCB callbackIn);
		static void performMeleeOnStartCallbacks();

		// OnMeleeReadyGo Callbacks
		static bool registerMeleeOnReadyGoCallback(MeleeOnReadyGoCB callbackIn);
		static bool unregisterMeleeOnReadyGoCallback(MeleeOnReadyGoCB callbackIn);
		static void performMeleeOnReadyGoCallbacks();

		// OnMeleeGameSet Callbacks
		static bool registerMeleeOnGameSetCallback(MeleeOnGameSetCB callbackIn);
		static bool unregisterMeleeOnGameSetCallback(MeleeOnGameSetCB callbackIn);
		static void performMeleeOnGameSetCallbacks();

		// OnCreate Callbacks
		static bool registerOnCreateCallback(FighterOnCreateCB callbackIn);
		static bool unregisterOnCreateCallback(FighterOnCreateCB callbackIn);
		static void performOnCreateCallbacks();

		// OnStart Callbacks
		static bool registerOnStartCallback(FighterOnStartCB callbackIn);
		static bool unregisterOnStartCallback(FighterOnStartCB callbackIn);
		static void performOnStartCallbacks();

		// OnRemove Callbacks
		static bool registerOnRemoveCallback(FighterOnRemoveCB callbackIn);
		static bool unregisterOnRemoveCallback(FighterOnRemoveCB callbackIn);
		static void performOnRemoveCallbacks();

		// Update Callbacks
		static bool registerOnUpdateCallback(FighterOnUpdateCB callbackIn);
		static bool unregisterOnUpdateCallback(FighterOnUpdateCB callbackIn);
		static void performOnUpdateCallbacks();

		// OnAttack Callbacks
		static bool registerOnAttackCallback(FighterOnAttackCB callbackIn);
		static bool unregisterOnAttackCallback(FighterOnAttackCB callbackIn);
		static bool registerOnAttackItemCallback(FighterOnAttackItemCB callbackIn);
		static bool unregisterOnAttackItemCallback(FighterOnAttackItemCB callbackIn);
		static bool registerOnAttackArticleCallback(FighterOnAttackArticleCB callbackIn);
		static bool unregisterOnAttackArticleCallback(FighterOnAttackArticleCB callbackIn);
		static void performOnAttackCallbacks();
	};
	void registerFighterHooks();
}

#endif
