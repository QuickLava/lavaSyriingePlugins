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
	const u32 maxFighterCount = 0x8;
	ftManager** const g_ftManagerPtrAddr = 0x80B87C28;

	typedef void (*FighterUpdateCB)(Fighter*);
	typedef void (*FighterOnCreateCB)(Fighter*);
	typedef void (*FighterOnStartCB)(Fighter*);
	typedef void (*FighterOnRemoveCB)(Fighter*);

	typedef bool (*FighterOnAttackCheckCB)(u32);
	typedef void (*FighterOnAttackCB)(float, soCollisionLog*, soModuleAccesser*);
	
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
		};
		static ftAttackWatcher m_attackWatchers[maxFighterCount];

		static Vector<FighterUpdateCB> m_updateCallbacks;
		static Vector<FighterOnCreateCB> m_onCreateCallbacks;
		static Vector<FighterOnStartCB> m_onStartCallbacks;
		static Vector<FighterOnRemoveCB> m_onRemoveCallbacks;
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
			OSReport("%sSubscribing watcher!\n", outputTag);
			watcherIn.setupObserver(fighterIn->m_moduleAccesser->getEventManageModule()->getManageId());
		}
		template<typename watcherType>
		static void unsubscribeWatcher(soEventObserver<watcherType>& watcherIn)
		{
			OSReport("%sUnsubscribing watcher!\n", outputTag);
			watcherIn.removeObserver();
		}

	public:
		// Update Callbacks
		static bool registerUpdateCallback(FighterUpdateCB callbackIn);
		static bool unregisterUpdateCallback(FighterUpdateCB callbackIn);
		static void performUpdateCallbacks();

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

		// OnAttack Callbacks
		static bool registerOnAttackCallback(FighterOnAttackCB callbackIn);
		static bool unregisterOnAttackCallback(FighterOnAttackCB callbackIn);
		static void performOnAttackCallbacks(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser);
	};
	void registerFighterHooks();
}

#endif