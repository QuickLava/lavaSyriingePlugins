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
	const char outputTag[] = "[fighterHooks] ";
	const char observerMessageFmt[] = "%s%s: [manageID: 0x%02X, unitID: 0x%02X, sendID: 0x%02X]!\n";

	const u32 maxFighterCount = 0x8;
	ftManager** const g_ftManagerPtrAddr = 0x80B87C28;
	GameGlobal** const g_GameGlobalPtrAddr = 0x805A00E0;

	typedef int (*gfTaskGetCategory)(gfTask*);
	const gfTaskGetCategory getCatPtr = (gfTaskGetCategory)0x8098C010;

	typedef void (*MeleeOnStartCB)();
	typedef void (*MeleeOnReadyGoCB)();
	typedef void (*MeleeOnGameSetCB)();
	typedef void (*FighterOnCreateCB)(Fighter*);
	typedef void (*FighterOnStartCB)(Fighter*);
	typedef void (*FighterOnRemoveCB)(Fighter*);
	typedef void (*FighterOnUpdateCB)(Fighter*);

	// Coll Categories:
	// - 0x02 = Enemies (Note: Bosses count)
	// - 0x06 = Yakumono (Note: Stage Hazards mostly, including Targets & Skyworld Plats)
	// - 0x0A = Fighter (Note: Nana counts!)
	// - 0x0B = Items (Note: Sandbag counts!)
	// - 0x0C = Weapon/Article (Note: Pikmin count!)
	typedef void (*FighterOnAttackCB)(Fighter*, StageObject*, float);
	typedef void (*FighterOnAttackItemCB)(Fighter*, StageObject*, float, BaseItem*);
	typedef void (*FighterOnAttackArticleCB)(Fighter*, StageObject*, float, Weapon*);

	class ftCallbackMgr
	{
	private:
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

		static Vector<MeleeOnStartCB> m_onMeleeStartCallbacks;
		static Vector<MeleeOnReadyGoCB> m_onMeleeReadyGoCallbacks;
		static Vector<MeleeOnGameSetCB> m_onMeleeGameSetCallbacks;
		static Vector<FighterOnCreateCB> m_onCreateCallbacks;
		static Vector<FighterOnStartCB> m_onStartCallbacks;
		static Vector<FighterOnRemoveCB> m_onRemoveCallbacks;
		static Vector<FighterOnUpdateCB> m_onUpdateCallbacks;
		static Vector<FighterOnAttackCB> m_onAttackCallbacks;
		static Vector<FighterOnAttackItemCB> m_onAttackItemCallbacks;
		static Vector<FighterOnAttackArticleCB> m_onAttackArticleCallbacks;

		static bool _registerCallback(Vector<void*>* targetVector, void* callbackIn);
		static bool _unregisterCallback(Vector<void*>* targetVector, void* callbackIn);
		template<typename cbType>
		static inline bool registerCallback(Vector<cbType>& targetVector, cbType callbackIn)
		{
			return _registerCallback((Vector<void*>*)&targetVector, (void*)callbackIn);
		}
		template<typename cbType>
		static inline bool unregisterCallback(Vector<cbType>& targetVector, cbType callbackIn)
		{
			return _unregisterCallback((Vector<void*>*)&targetVector, (void*)callbackIn);
		}
		
		static void subscribeEventWatcher();
		static void unsubscribeEventWatcher();

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
