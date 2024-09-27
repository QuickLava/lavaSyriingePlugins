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
	typedef void (*FighterUpdateCB)(Fighter*);
	typedef void (*FighterOnStartCB)(Fighter*);

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
		static ftAttackWatcher m_attackWatcher;

		static Vector<FighterUpdateCB> m_updateCallbacks;
		static Vector<FighterOnStartCB> m_onStartCallbacks;
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
			if (fighterIn == NULL || fighterIn->m_moduleAccesser->getEventManageModule() == NULL) return;

			soEventSystem* eventSys = soEventSystem::getInstance();
			soEventManageModule* eventModule = fighterIn->m_moduleAccesser->getEventManageModule();
			if (eventModule == NULL) return;

			int sendId = -1;
			int manageId = eventModule->getManageId();
			if ((-1 < manageId) && (-1 < watcherIn.m_unitID) && eventSys->m_instanceManager.isContain(manageId))
			{
				soEventManager* eventManager = eventSys->getManager(manageId);
				if (!eventManager->isNullUnit(watcherIn.m_unitID))
				{
					if (eventManager->isExist(watcherIn.m_unitID))
					{
						soEventUnitWrapper<soCollisionAttackEventObserver>* eventUnit =
							dynamic_cast<soEventUnitWrapper<soCollisionAttackEventObserver>*>(eventManager->getEventUnit(watcherIn.m_unitID));
						if (eventUnit != NULL)
						{
							sendId = eventUnit->addObserverSub(static_cast<soCollisionAttackEventObserver*>(&watcherIn), -1);
							OSReport("%sRegistered Fighter [Name: %s]!\n", outputTag, fighterIn->m_taskName);
							OSReport("%sObserver IDs [ManagerID: %02X, UnitID: %02X, SendID: %02X]!\n", 
								outputTag, watcherIn.m_manageID, watcherIn.m_unitID, sendId);
						}
					}
					watcherIn.m_sendID = sendId;
					if (-1 < sendId)
					{
						watcherIn.m_manageID = manageId;
					}
				}
			}
		}

	public:
		// Update Callbacks
		static bool registerUpdateCallback(FighterUpdateCB callbackIn);
		static bool unregisterUpdateCallback(FighterUpdateCB callbackIn);
		static void performUpdateCallbacks();

		// OnStart Callbacks
		static bool registerOnStartCallback(FighterOnStartCB callbackIn);
		static bool unregisterOnStartCallback(FighterOnStartCB callbackIn);
		static void performOnStartCallbacks();

		// OnAttack Callbacks
		static bool registerOnAttackCallback(FighterOnAttackCB callbackIn);
		static bool unregisterOnAttackCallback(FighterOnAttackCB callbackIn);
		static void performOnAttackCallbacks(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser);
	};
	void registerFighterHooks();
}

#endif
