#ifndef LAVA_FIGHTER_HOOKS_H_V1
#define LAVA_FIGHTER_HOOKS_H_V1

#include <sy_core.h>
#include <string.h>
#include <vector.h>
#include <modules.h>
#include <ft/fighter.h>
#include <ft/ft_manager.h>
#include <os/OSError.h>
#include <so/event/so_event_observer.h>

namespace fighterHooks
{
	const char outputTag[] = "[fighterHooks] ";
	const char observerMessageFmt[] = "%s%s: [manageID: 0x%02X, unitID: 0x%02X, sendID: 0x%02X]!\n";

	const u32 maxFighterCount = 0x8;
	ftManager** const g_ftManagerPtrAddr = 0x80B87C28;

	typedef void (*MeleeOnReadyGoCB)();
	typedef void (*MeleeOnGameSetCB)();
	typedef void (*FighterOnCreateCB)(Fighter*);
	typedef void (*FighterOnStartCB)(Fighter*);
	typedef void (*FighterOnRemoveCB)(Fighter*);
	typedef void (*FighterOnUpdateCB)(Fighter*);

	typedef bool (*FighterOnAttackCheckCB)(u32);
	// Coll Categories:
	// - 0x02 = Enemies (Note: Bosses count)
	// - 0x06 = Yakumono (Note: Stage Hazards mostly, including Targets & Skyworld Plats)
	// - 0x0A = Fighter (Note: Nana counts!)
	// - 0x0B = Items (Note: Sandbag counts!)
	// - 0x0C = Weapon/Article (Note: Pikmin count!)
	typedef void (*FighterOnAttackCB)(Fighter*, StageObject*, float, soCollisionLog*);
	
	class ftCallbackMgr
	{
	private:

		class ftAttackWatcher : public soCollisionAttackEventObserver
		{
		public:
			ftAttackWatcher();
			virtual ~ftAttackWatcher();

			virtual void addObserver(short param1, s8 param2);
			virtual bool notifyEventCollisionAttackCheck(u32 flags);
			virtual void notifyEventCollisionAttack(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser);

			StageObject* getStageObjFromCollLog(soCollisionLog* collisionLog);
		};
		static ftAttackWatcher m_attackWatchers[maxFighterCount];

		static Vector<MeleeOnReadyGoCB> m_onReadyGoCallbacks;
		static Vector<MeleeOnGameSetCB> m_onGameSetCallbacks;
		static Vector<FighterOnCreateCB> m_onCreateCallbacks;
		static Vector<FighterOnStartCB> m_onStartCallbacks;
		static Vector<FighterOnRemoveCB> m_onRemoveCallbacks;
		static Vector<FighterOnUpdateCB> m_onUpdateCallbacks;
		static Vector<FighterOnAttackCB> m_onAttackCallbacks;

		template<typename cbType>
		static bool registerCallback(Vector<cbType>& targetVector, cbType callbackIn)
		{
			bool result = 1;

			for (int i = 0; result && i < targetVector.size(); i++)
			{
				result = targetVector[i] != callbackIn;
			}

			if (result)
			{
				targetVector.push(callbackIn);
			}

			return result;
		}
		template<typename cbType>
		static bool unregisterCallback(Vector<cbType>& targetVector, cbType callbackIn)
		{
			bool result = 0;

			for (int i = 0; !result && i < targetVector.size(); i++)
			{
				result = targetVector[i] == callbackIn;
				if (result && ((i + 1) < targetVector.size()))
				{
					targetVector[i] = targetVector[i + 1];
				}
			}

			if (result)
			{
				targetVector.pop();
			}

			return result;
		}
		template<typename watcherType>
		static void subscribeWatcherToFighter(soEventObserver<watcherType>& watcherIn, Fighter* fighterIn)
		{
			watcherIn.setupObserver(fighterIn->m_moduleAccesser->getEventManageModule()->getManageId());
			OSReport(observerMessageFmt, outputTag, "Subscribed Watcher", watcherIn.m_manageID, watcherIn.m_unitID, watcherIn.m_sendID);
		}
		template<typename watcherType>
		static void unsubscribeWatcher(soEventObserver<watcherType>& watcherIn)
		{
			OSReport(observerMessageFmt, outputTag, "Unsubscribing Watcher", watcherIn.m_manageID, watcherIn.m_unitID, watcherIn.m_sendID);
			watcherIn.removeObserver();
		}

	public:
		// OnReadyGo Callbacks
		static bool registerOnReadyGoCallback(MeleeOnReadyGoCB callbackIn);
		static bool unregisterOnReadyGoCallback(MeleeOnReadyGoCB callbackIn);
		static void performOnReadyGoCallbacks();

		// OnGameSet Callbacks
		static bool registerOnGameSetCallback(MeleeOnGameSetCB callbackIn);
		static bool unregisterOnGameSetCallback(MeleeOnGameSetCB callbackIn);
		static void performOnGameSetCallbacks();

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
		static void performOnAttackCallbacks(Fighter* attacker, StageObject* target, float power, soCollisionLog* collisionLog);
	};
	void registerFighterHooks();
}

#endif
