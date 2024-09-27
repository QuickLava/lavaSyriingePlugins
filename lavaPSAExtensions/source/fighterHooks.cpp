#include "fighterHooks.h"

namespace fighterHooks
{
	const char callbackHookMsgFmt[] = "%s%s: entryID = 0x%08X, playerID = 0x%02X!\n";

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
		ftCallbackMgr::performOnAttackCallbacks((Fighter*)moduleAccesser->m_stageObject, getStageObjFromCollLog(collisionLog), power, collisionLog);
	}
	StageObject* ftCallbackMgr::ftAttackWatcher::getStageObjFromCollLog(soCollisionLog* collisionLog)
	{
		typedef StageObject* (*getTaskByIDPtr)(void*, u32, int);
		void** const g_gfTaskSchedulerPtrAddr = (void**)0x805A0068;
		const getTaskByIDPtr getTaskByID = (getTaskByIDPtr)0x8002F018;

		return (StageObject*)getTaskByID(*g_gfTaskSchedulerPtrAddr, collisionLog->m_category, collisionLog->m_taskId);
	}

	ftCallbackMgr::ftAttackWatcher ftCallbackMgr::m_attackWatchers[maxFighterCount];
	Vector<FighterOnCreateCB> ftCallbackMgr::m_onCreateCallbacks;
	Vector<FighterOnStartCB> ftCallbackMgr::m_onStartCallbacks;
	Vector<FighterOnRemoveCB> ftCallbackMgr::m_onRemoveCallbacks;
	Vector<FighterOnUpdateCB> ftCallbackMgr::m_onUpdateCallbacks;
	Vector<FighterOnAttackCB> ftCallbackMgr::m_onAttackCallbacks;

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

		OSReport(callbackHookMsgFmt, outputTag, "OnCreate Callbacks", entryID, manager->getPlayerNo(entryID));

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

		OSReport(callbackHookMsgFmt, outputTag, "OnStart Callbacks", entryID, manager->getPlayerNo(entryID));

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

		OSReport(callbackHookMsgFmt, outputTag, "OnRemove Callbacks", entryID, manager->getPlayerNo(entryID));

		// Execute Callbacks
		Fighter* fighter = manager->getFighter(entryID, -1);
		for (int i = 0; i < m_onRemoveCallbacks.size(); i++)
		{
			m_onRemoveCallbacks[i](fighter);
		}

		// Unsubscribe Watchers
		unsubscribeWatcher(m_attackWatchers[manager->getPlayerNo(entryID)]);
	}

	bool ftCallbackMgr::registerOnUpdateCallback(FighterOnUpdateCB callbackIn)
	{
		return registerCallback<FighterOnUpdateCB>(m_onUpdateCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnUpdateCallback(FighterOnUpdateCB callbackIn)
	{
		return unregisterCallback<FighterOnUpdateCB>(m_onUpdateCallbacks, callbackIn);
	}
	void ftCallbackMgr::performOnUpdateCallbacks()
	{
		register Fighter* fighter;
		asm
		{
			mr fighter, r29
		}

		for (int i = 0; i < m_onUpdateCallbacks.size(); i++)
		{
			m_onUpdateCallbacks[i](fighter);
		}
	}

	bool ftCallbackMgr::registerOnAttackCallback(FighterOnAttackCB callbackIn)
	{
		return registerCallback<FighterOnAttackCB>(m_onAttackCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnAttackCallback(FighterOnAttackCB callbackIn)
	{
		return unregisterCallback<FighterOnAttackCB>(m_onAttackCallbacks, callbackIn);
	}
	void ftCallbackMgr::performOnAttackCallbacks(Fighter* attacker, StageObject* target, float power, soCollisionLog* collisionLog)
	{
		OSReport("%sOnAttack Callbacks!\n", outputTag);
		OSReport("%s- Target: %s, TaskID: 0x%04X, Category: 0x%04X!\n",
			outputTag, target->m_taskName, collisionLog->m_taskId, collisionLog->m_category);

		for (int i = 0; i < m_onAttackCallbacks.size(); i++)
		{
			m_onAttackCallbacks[i](attacker, target, power, collisionLog);
		}
	}

	void registerFighterHooks()
	{
		// General Fighter Create Hook @ 0x80814358: 0x24 bytes into symbol "createFighter/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109944, reinterpret_cast<void*>(ftCallbackMgr::performOnCreateCallbacks), Modules::SORA_MELEE);

		// General Fighter Start Hook @ 0x80814774: 0x10 bytes into symbol "startFighter/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109D60, reinterpret_cast<void*>(ftCallbackMgr::performOnStartCallbacks), Modules::SORA_MELEE);

		// General Fighter Exit Hook @ 0x80814384: 0x14 bytes into symbol "removeEntry/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109970, reinterpret_cast<void*>(ftCallbackMgr::performOnRemoveCallbacks), Modules::SORA_MELEE);

		// General Fighter Update Hook @ 0x80839160: 0xAA4 bytes into symbol "processUpdate/[Fighter]/fighter.o"
		SyringeCore::syInlineHookRel(0x12E74C, reinterpret_cast<void*>(ftCallbackMgr::performOnUpdateCallbacks), Modules::SORA_MELEE);
	}
}
