#include "fighterHooks.h"

namespace fighterHooks
{
	const char callbackHookMsgFmt[] = "%s%s: entryID = 0x%08X, playerID = 0x%02X!\n";

	ftCallbackMgr::ftEventWatcher::ftEventWatcher() : ftOutsideEventObserver() {}
	void ftCallbackMgr::ftEventWatcher::notifyEventAppeal(int entryId, int)
	{
		OSReport_N("%sEventAppeal\n", outputTag);
	}
	void ftCallbackMgr::ftEventWatcher::notifyEventDead(int entryId, int deadCount, int, int)
	{
		OSReport_N("%sEventDead\n", outputTag);
	}
	void ftCallbackMgr::ftEventWatcher::notifyEventSuicide(int entryId)
	{
		OSReport_N("%sEventSuicide\n", outputTag);
	}
	void ftCallbackMgr::ftEventWatcher::notifyEventSucceedHit(int entryId, u32 consecutiveHits, float totalDamage)
	{
		OSReport_N("%sEventSucceedHit, %02d Hits!\n", outputTag, consecutiveHits);
	}
	void ftCallbackMgr::ftEventWatcher::notifyEventKnockout(int entryId)
	{
		OSReport_N("%sEventKnockout\n", outputTag);
	}
	void ftCallbackMgr::ftEventWatcher::notifyEventBeat(int entryId1, int entryId2)
	{
		OSReport_N("%sEventBeat, ID1: 0x%08X, ID2: 0x%08X\n", outputTag, entryId1, entryId2);
	}

	ftCallbackMgr::ftAttackWatcher::ftAttackWatcher() : soCollisionAttackEventObserver(1) {}
	ftCallbackMgr::ftAttackWatcher::~ftAttackWatcher() {}
	void ftCallbackMgr::ftAttackWatcher::addObserver(short param1, s8 param2)
	{
		return;
	}
	bool ftCallbackMgr::ftAttackWatcher::notifyEventCollisionAttackCheck(u32 flags)
	{
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

	ftCallbackMgr::ftEventWatcher ftCallbackMgr::m_eventWatcher;
	ftCallbackMgr::ftAttackWatcher ftCallbackMgr::m_attackWatchers[maxFighterCount];
	Vector<MeleeOnStartCB> ftCallbackMgr::m_onMeleeStartCallbacks;
	Vector<MeleeOnReadyGoCB> ftCallbackMgr::m_onMeleeReadyGoCallbacks;
	Vector<MeleeOnGameSetCB> ftCallbackMgr::m_onMeleeGameSetCallbacks;
	Vector<FighterOnCreateCB> ftCallbackMgr::m_onCreateCallbacks;
	Vector<FighterOnStartCB> ftCallbackMgr::m_onStartCallbacks;
	Vector<FighterOnRemoveCB> ftCallbackMgr::m_onRemoveCallbacks;
	Vector<FighterOnUpdateCB> ftCallbackMgr::m_onUpdateCallbacks;
	Vector<FighterOnAttackCB> ftCallbackMgr::m_onAttackCallbacks;

	void ftCallbackMgr::subscribeEventWatcher()
	{
		int ftManagerManageID = (*g_ftManagerPtrAddr)->m_eventManageModule.getManageId();

		ftOutsideEventObserver* watcher_Out = (ftOutsideEventObserver*)(&m_eventWatcher);
		watcher_Out->setupObserver(ftManagerManageID);
		OSReport_N(observerMessageFmt, outputTag, "Subscribed Event Watcher_Out", watcher_Out->m_manageID, watcher_Out->m_unitID, watcher_Out->m_sendID);
	}
	void ftCallbackMgr::unsubscribeEventWatcher()
	{
		ftOutsideEventObserver* watcher_Out = (ftOutsideEventObserver*)(&m_eventWatcher);
		OSReport_N(observerMessageFmt, outputTag, "Unsubscribing Event Watcher_Out", watcher_Out->m_manageID, watcher_Out->m_unitID, watcher_Out->m_sendID);
		watcher_Out->removeObserver();
	}

	// MeleeOnStart Callbacks
	bool ftCallbackMgr::registerMeleeOnStartCallback(MeleeOnStartCB callbackIn)
	{
		return registerCallback<MeleeOnStartCB>(m_onMeleeStartCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterMeleeOnStartCallback(MeleeOnStartCB callbackIn)
	{
		return unregisterCallback<MeleeOnStartCB>(m_onMeleeStartCallbacks, callbackIn);
	}
	void ftCallbackMgr::performMeleeOnStartCallbacks()
	{
		OSReport_N("%sOnMeleeStart Callbacks\n", outputTag);

		// Execute Callbacks
		for (int i = 0; i < m_onMeleeStartCallbacks.size(); i++)
		{
			m_onMeleeStartCallbacks[i]();
		}

		// Subscribe Event Watcher
		subscribeEventWatcher();
	}

	// MeleeOnReadyGo Callbacks
	bool ftCallbackMgr::registerMeleeOnReadyGoCallback(MeleeOnReadyGoCB callbackIn)
	{
		return registerCallback<MeleeOnReadyGoCB>(m_onMeleeReadyGoCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterMeleeOnReadyGoCallback(MeleeOnReadyGoCB callbackIn)
	{
		return unregisterCallback<MeleeOnReadyGoCB>(m_onMeleeReadyGoCallbacks, callbackIn);
	}
	void ftCallbackMgr::performMeleeOnReadyGoCallbacks()
	{
		OSReport_N("%sOnMeleeReadyGo Callbacks\n", outputTag);

		// Execute Callbacks
		for (int i = 0; i < m_onMeleeReadyGoCallbacks.size(); i++)
		{
			m_onMeleeReadyGoCallbacks[i]();
		}
	}

	// MeleeOnGameSet Callbacks
	bool ftCallbackMgr::registerMeleeOnGameSetCallback(MeleeOnGameSetCB callbackIn)
	{
		return registerCallback<MeleeOnGameSetCB>(m_onMeleeGameSetCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterMeleeOnGameSetCallback(MeleeOnGameSetCB callbackIn)
	{
		return unregisterCallback<MeleeOnGameSetCB>(m_onMeleeGameSetCallbacks, callbackIn);
	}
	void ftCallbackMgr::performMeleeOnGameSetCallbacks()
	{
		OSReport_N("%sOnMeleeGameSet Callbacks\n", outputTag);

		// Execute Callbacks
		for (int i = 0; i < m_onMeleeGameSetCallbacks.size(); i++)
		{
			m_onMeleeGameSetCallbacks[i]();
		}

		// Unsubscribe Event Watcher
		unsubscribeEventWatcher();
	}

	// OnCreate Callbacks
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

		OSReport_N(callbackHookMsgFmt, outputTag, "OnCreate Callbacks", entryID, manager->getPlayerNo(entryID));

		// Execute Callbacks
		Fighter* fighter = manager->getFighter(entryID, -1);
		for (int i = 0; i < m_onCreateCallbacks.size(); i++)
		{
			m_onCreateCallbacks[i](fighter);
		}

		// Subscribe Watchers
		subscribeWatcherToFighter(m_attackWatchers[manager->getPlayerNo(entryID)], fighter);
	}

	// OnStart Callbacks
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

		OSReport_N(callbackHookMsgFmt, outputTag, "OnStart Callbacks", entryID, manager->getPlayerNo(entryID));

		// Execute Callbacks
		Fighter* fighter = manager->getFighter(entryID, -1);
		for (int i = 0; i < m_onStartCallbacks.size(); i++)
		{
			m_onStartCallbacks[i](fighter);
		}
	}
	
	// OnRemove Callbacks
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

		OSReport_N(callbackHookMsgFmt, outputTag, "OnRemove Callbacks", entryID, manager->getPlayerNo(entryID));

		// Execute Callbacks
		Fighter* fighter = manager->getFighter(entryID, -1);
		for (int i = 0; i < m_onRemoveCallbacks.size(); i++)
		{
			m_onRemoveCallbacks[i](fighter);
		}

		// Unsubscribe Watchers
		unsubscribeWatcher(m_attackWatchers[manager->getPlayerNo(entryID)]);
	}

	// Update Callbacks
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

	// OnAttack Callbacks
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
		OSReport_N("%sOnAttack Callbacks!\n", outputTag);
		OSReport_N("%s- Attacker: %s, TaskID: 0x%08X\n", outputTag, attacker->m_taskName, attacker->m_taskId);
		if (target != NULL)
		{
			OSReport_N("%s-   Target: %s, TaskID: 0x%08X, Category: 0x%02X!\n",
				outputTag, target->m_taskName, collisionLog->m_taskId, collisionLog->m_category);
		}
		else
		{
			OSReport_N("%s-   Target: NULL\n");
		}

		for (int i = 0; i < m_onAttackCallbacks.size(); i++)
		{
			m_onAttackCallbacks[i](attacker, target, power, collisionLog);
		}
	}

	void registerFighterHooks()
	{
		// Match Start Hook @ 0x80813D24: 0x08 bytes into symbol "start/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109310, reinterpret_cast<void*>(ftCallbackMgr::performMeleeOnStartCallbacks), Modules::SORA_MELEE);

		// Match Countdown GO! Hook @ 0x80813D70: 0x44 bytes into symbol "readyGo/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x10935C, reinterpret_cast<void*>(ftCallbackMgr::performMeleeOnReadyGoCallbacks), Modules::SORA_MELEE);

		// Match GameSet Hook @ 0x80813E1C: 0xA4 bytes into symbol "gameSet/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109408, reinterpret_cast<void*>(ftCallbackMgr::performMeleeOnGameSetCallbacks), Modules::SORA_MELEE);

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
