#include "fighterHooks.h"

namespace fighterHooks
{
	ftCallbackMgr::ftAttackWatcher::ftAttackWatcher() : soCollisionAttackEventObserver(1) {}
	ftCallbackMgr::ftAttackWatcher::~ftAttackWatcher() {}
	void ftCallbackMgr::ftAttackWatcher::addObserver(short param1, s8 param2)
	{
		return;
	}
	bool ftCallbackMgr::ftAttackWatcher::notifyEventCollisionAttackCheck(u32 flags)
	{
		OSReport("%sCollisionAttackCheck!\n", outputTag);
		return 0;
	}
	void ftCallbackMgr::ftAttackWatcher::notifyEventCollisionAttack(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser)
	{
		OSReport("%sCollisionAttack!\n", outputTag);
		ftCallbackMgr::performOnAttackCallbacks(power, collisionLog, moduleAccesser);
		return;
	}

	ftCallbackMgr::ftAttackWatcher ftCallbackMgr::m_attackWatchers[maxFighterCount];
	Vector<FighterUpdateCB> ftCallbackMgr::m_updateCallbacks;
	Vector<FighterOnCreateCB> ftCallbackMgr::m_onCreateCallbacks;
	Vector<FighterOnStartCB> ftCallbackMgr::m_onStartCallbacks;
	Vector<FighterOnRemoveCB> ftCallbackMgr::m_onRemoveCallbacks;
	Vector<FighterOnAttackCB> ftCallbackMgr::m_onAttackCallbacks;

	bool ftCallbackMgr::registerUpdateCallback(FighterUpdateCB callbackIn)
	{
		return registerCallback<FighterUpdateCB>(m_updateCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterUpdateCallback(FighterUpdateCB callbackIn)
	{
		return unregisterCallback<FighterUpdateCB>(m_updateCallbacks, callbackIn);
	}
	void ftCallbackMgr::performUpdateCallbacks()
	{
		register Fighter* fighter;
		asm
		{
			mr fighter, r29
		}
		
		for (int i = 0; i < m_updateCallbacks.size(); i++)
		{
			m_updateCallbacks[i](fighter);
		}
	}

	bool ftCallbackMgr::registerOnCreateCallback(FighterOnCreateCB callbackIn)
	{
		return registerCallback<FighterOnCreateCB>(m_onCreateCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnCreateCallback(FighterOnCreateCB callbackIn)
	{
		return unregisterCallback<FighterOnCreateCB>(m_onCreateCallbacks, callbackIn);
	}
	void ftCallbackMgr::performOnCreateCallbacks()
	{
		ftManager* manager = *g_ftManagerPtrAddr;
		register u32 entryID;
		asm
		{
			lwz entryID, 0x04(r31);
		}

		OSReport("%sOnCreate Callbacks: entryID = 0x%08X, playerID = 0x%08X!\n", outputTag, entryID, manager->getPlayerNo(entryID));

		// Execute Callbacks
		Fighter* fighter = manager->getFighter(entryID, -1);
		for (int i = 0; i < m_onCreateCallbacks.size(); i++)
		{
			m_onCreateCallbacks[i](fighter);
		}

		// Subscribe Watchers
		subscribeWatcherToFighter(m_attackWatchers[manager->getPlayerNo(entryID)], fighter);
	}

	bool ftCallbackMgr::registerOnStartCallback(FighterOnStartCB callbackIn)
	{
		return registerCallback<FighterOnStartCB>(m_onStartCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnStartCallback(FighterOnStartCB callbackIn)
	{
		return unregisterCallback<FighterOnStartCB>(m_onStartCallbacks, callbackIn);
	}
	void ftCallbackMgr::performOnStartCallbacks()
	{
		register ftManager* manager;
		register u32 entryID;
		asm
		{
			mr manager, r3;
			mr entryID, r4;
		}

		OSReport("%sOnStart Callbacks: entryID = 0x%08X, playerID = 0x%08X!\n", outputTag, entryID, manager->getPlayerNo(entryID));

		// Execute Callbacks
		Fighter* fighter = manager->getFighter(entryID, -1);
		for (int i = 0; i < m_onStartCallbacks.size(); i++)
		{
			m_onStartCallbacks[i](fighter);
		}
	}
	
	bool ftCallbackMgr::registerOnRemoveCallback(FighterOnRemoveCB callbackIn)
	{
		return registerCallback<FighterOnRemoveCB>(m_onRemoveCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnRemoveCallback(FighterOnRemoveCB callbackIn)
	{
		return unregisterCallback<FighterOnRemoveCB>(m_onRemoveCallbacks, callbackIn);
	}
	void ftCallbackMgr::performOnRemoveCallbacks()
	{
		register ftManager* manager;
		register u32 entryID;
		asm
		{
			mr manager, r3;
			mr entryID, r4;
		}

		OSReport("%sOnRemove Callbacks: entryID = 0x%08X, playerID = 0x%08X!\n", outputTag, entryID, manager->getPlayerNo(entryID));

		// Execute Callbacks
		Fighter* fighter = manager->getFighter(entryID, -1);
		for (int i = 0; i < m_onRemoveCallbacks.size(); i++)
		{
			m_onRemoveCallbacks[i](fighter);
		}

		// Unsubscribe Watchers
		unsubscribeWatcher(m_attackWatchers[manager->getPlayerNo(entryID)]);
	}

	bool ftCallbackMgr::registerOnAttackCallback(FighterOnAttackCB callbackIn)
	{
		return registerCallback<FighterOnAttackCB>(m_onAttackCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnAttackCallback(FighterOnAttackCB callbackIn)
	{
		return unregisterCallback<FighterOnAttackCB>(m_onAttackCallbacks, callbackIn);
	}
	void ftCallbackMgr::performOnAttackCallbacks(float power, soCollisionLog* collisionLog, soModuleAccesser* moduleAccesser)
	{
		OSReport("%sOnAttack Callbacks!\n", outputTag);
		for (int i = 0; i < m_onAttackCallbacks.size(); i++)
		{
			m_onAttackCallbacks[i](power, collisionLog, moduleAccesser);
		}
	}

	void registerFighterHooks()
	{
		// General Fighter Update Hook @ 0x80839160: 0xAA4 bytes into symbol "processUpdate/[Fighter]/fighter.o"
		SyringeCore::syInlineHookRel(0x12E74C, reinterpret_cast<void*>(ftCallbackMgr::performUpdateCallbacks), Modules::SORA_MELEE);

		// General Fighter Create Hook @ 0x80814358: 0x24 bytes into symbol "createFighter/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109944, reinterpret_cast<void*>(ftCallbackMgr::performOnCreateCallbacks), Modules::SORA_MELEE);

		// General Fighter Start Hook @ 0x80814774: 0x10 bytes into symbol "startFighter/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109D60, reinterpret_cast<void*>(ftCallbackMgr::performOnStartCallbacks), Modules::SORA_MELEE);

		// General Fighter Exit Hook @ 0x80814384: 0x14 bytes into symbol "removeEntry/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109970, reinterpret_cast<void*>(ftCallbackMgr::performOnRemoveCallbacks), Modules::SORA_MELEE);
	}
}
