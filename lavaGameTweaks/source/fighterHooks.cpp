#include "fighterHooks.h"

namespace fighterHooks
{
	const char outputTag[] = "[fighterHooks] ";
	const char observerMessageFmt[] = "%s%s: [manageID: 0x%02X, unitID: 0x%02X, sendID: 0x%02X]!\n";
	const char callbackHookMsgFmt[] = "%s%s: entryID = 0x%08X, playerNo = 0x%02X!\n";

	u32 getFighterSlotNo(Fighter* fighterIn)
	{
		return fighterIn->m_entryId & 0xFF;
	}
	u32 getFighterPlayerNo(Fighter* fighterIn)
	{
		return g_ftManager->getPlayerNo(fighterIn->m_entryId);
	}

#if INCLUDE_OUTSIDE_OBSERVER
	ftCallbackMgr::ftEventWatcher ftCallbackMgr::m_eventWatcher;
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
#endif

	Vector<void*> ftCallbackMgr::m_onMeleeStartCallbacks;
	Vector<void*> ftCallbackMgr::m_onMeleeReadyGoCallbacks;
	Vector<void*> ftCallbackMgr::m_onMeleeGameSetCallbacks;
	Vector<void*> ftCallbackMgr::m_onCreateCallbacks;
	Vector<void*> ftCallbackMgr::m_onStartCallbacks;
	Vector<void*> ftCallbackMgr::m_onRemoveCallbacks;
	Vector<void*> ftCallbackMgr::m_onUpdateCallbacks;
	Vector<void*> ftCallbackMgr::m_onHitCallbacks;
	Vector<void*> ftCallbackMgr::m_onAttackCallbacks;
	Vector<void*> ftCallbackMgr::m_onAttackItemCallbacks;
	Vector<void*> ftCallbackMgr::m_onAttackArticleCallbacks;

	bool ftCallbackMgr::_registerCallback(Vector<void*>* targetVector, void* callbackIn)
	{
		bool result = 1;

		for (int i = 0; result && i < targetVector->size(); i++)
		{
			result = (*targetVector)[i] != callbackIn;
		}

		if (result)
		{
			targetVector->push(callbackIn);
		}

		return result;
	}
	bool ftCallbackMgr::_unregisterCallback(Vector<void*>* targetVector, void* callbackIn)
	{
		bool result = 0;

		for (int i = 0; !result && i < targetVector->size(); i++)
		{
			result = (*targetVector)[i] == callbackIn;
			if (result && ((i + 1) < targetVector->size()))
			{
				(*targetVector)[i] = (*targetVector)[i + 1];
			}
		}

		if (result)
		{
			targetVector->pop();
		}

		return result;
	}
	void ftCallbackMgr::_performArglessCallbacks(Vector<void*>* targetVector)
	{
		// Execute Callbacks
		for (int i = 0; i < targetVector->size(); i++)
		{
			GenericArglessCB currCallback = (GenericArglessCB)(*targetVector)[i];
			currCallback();
		}
	}
	void ftCallbackMgr::_performFighterEventCallbacks(Vector<void*>* targetVector, u32 entryID)
	{
		// Execute Callbacks
		Fighter* fighterIn = g_ftManager->getFighter(entryID, -1);
		for (int i = 0; i < targetVector->size(); i++)
		{
			GenericFighterEventCB currCallback = (GenericFighterEventCB)(*targetVector)[i];
			currCallback(fighterIn);
		}
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
		_performArglessCallbacks(&m_onMeleeStartCallbacks);

#if INCLUDE_OUTSIDE_OBSERVER
		// Subscribe Event Watcher
		subscribeEventWatcher();
#endif
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
		_performArglessCallbacks(&m_onMeleeReadyGoCallbacks);
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
		_performArglessCallbacks(&m_onMeleeGameSetCallbacks);

#if INCLUDE_OUTSIDE_OBSERVER
		// Unsubscribe Event Watcher
		unsubscribeEventWatcher();
#endif
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
		register u32 entryID;
		asm
		{
			lwz entryID, 0x04(r31);
		}

		OSReport_N(callbackHookMsgFmt, outputTag, "OnCreate Callbacks", entryID, g_ftManager->getPlayerNo(entryID));

		// Execute Callbacks
		_performFighterEventCallbacks(&m_onCreateCallbacks, entryID);
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
		register u32 entryID;
		asm
		{
			mr entryID, r4;
		}

		OSReport_N(callbackHookMsgFmt, outputTag, "OnStart Callbacks", entryID, g_ftManager->getPlayerNo(entryID));

		// Execute Callbacks
		_performFighterEventCallbacks(&m_onStartCallbacks, entryID);
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
		register u32 entryID;
		asm
		{
			mr entryID, r4;
		}

		OSReport_N(callbackHookMsgFmt, outputTag, "OnRemove Callbacks", entryID, g_ftManager->getPlayerNo(entryID));

		// Execute Callbacks
		_performFighterEventCallbacks(&m_onRemoveCallbacks, entryID);
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

		// Execute Callbacks
		_performFighterEventCallbacks(&m_onUpdateCallbacks, fighter->m_entryId);
	}


	// OnHit Callbacks
	bool ftCallbackMgr::registerOnHitCallback(FighterOnHitCB callbackIn)
	{
		return registerCallback<FighterOnHitCB>(m_onHitCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnHitCallback(FighterOnHitCB callbackIn)
	{
		return unregisterCallback<FighterOnHitCB>(m_onHitCallbacks, callbackIn);
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
	bool ftCallbackMgr::registerOnAttackItemCallback(FighterOnAttackItemCB callbackIn)
	{
		return registerCallback<FighterOnAttackItemCB>(m_onAttackItemCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnAttackItemCallback(FighterOnAttackItemCB callbackIn)
	{
		return unregisterCallback<FighterOnAttackItemCB>(m_onAttackItemCallbacks, callbackIn);
	}
	bool ftCallbackMgr::registerOnAttackArticleCallback(FighterOnAttackArticleCB callbackIn)
	{
		return registerCallback<FighterOnAttackArticleCB>(m_onAttackArticleCallbacks, callbackIn);
	}
	bool ftCallbackMgr::unregisterOnAttackArticleCallback(FighterOnAttackArticleCB callbackIn)
	{
		return unregisterCallback<FighterOnAttackArticleCB>(m_onAttackArticleCallbacks, callbackIn);
	}
	void ftCallbackMgr::performOnAttackCallbacks()
	{
		const char fmtStr1[] = "%s- %8s: %s, TaskID: 0x%08X, Category: 0x%04X\n";

		register gfTask* attackerTask;
		register gfTask* attackerParentTask;
		register gfTask* targetTask;
		register int unsure;
		register float damageDealt;

		asm
		{
			mr attackerTask, r31;
			mr attackerParentTask, r30;
			mr targetTask, r29;
			mr unsure, r25;
			fmr damageDealt, f31;
		}

		gfTask::Category attackerCategory = attackerTask->m_taskCategory;
		gfTask::Category targetCategory = targetTask->m_taskCategory;

		OSReport_N("%sOnAttack & OnHit Callbacks: ??? 0x%02X, Damage: %2.0f\%!\n", outputTag, unsure, damageDealt);
		OSReport_N(fmtStr1, outputTag, "Attacker", attackerTask->m_taskName, attackerTask->m_taskId, attackerCategory);
		if (attackerParentTask != NULL && attackerParentTask != attackerTask)
		{
			OSReport_N(fmtStr1, outputTag, "Parent", attackerParentTask->m_taskName, attackerParentTask->m_taskId, getCatPtr(attackerParentTask));
		}
		if (targetTask != NULL)
		{
			OSReport_N(fmtStr1, outputTag, "Target", targetTask->m_taskName, targetTask->m_taskId, getCatPtr(targetTask));
		}
		else
		{
			OSReport_N("%s-   Target: NULL\n");
		}

		Vector<void*>* callbackVector;
		if (attackerCategory == gfTask::Category_Fighter)
		{
			callbackVector = &m_onAttackCallbacks;
		}
		else if (attackerParentTask != NULL)
		{
			gfTask* temp = attackerTask;
			attackerTask = attackerParentTask;
			attackerParentTask = temp;
			if (attackerCategory == gfTask::Category_Item)
			{
				callbackVector = &m_onAttackItemCallbacks;
			}
			else if (attackerCategory == gfTask::Category_Weapon)
			{
				callbackVector = &m_onAttackArticleCallbacks;
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}
		for (int i = 0; i < callbackVector->size(); i++)
		{
			FighterOnAttackGenericCB currCallback = (FighterOnAttackGenericCB)(*callbackVector)[i];
			currCallback((Fighter*)attackerTask, (StageObject*)targetTask, damageDealt, (StageObject*)attackerParentTask);
		}

		if (targetCategory == gfTask::Category_Fighter)
		{
			for (int i = 0; i < m_onHitCallbacks.size(); i++)
			{
				((FighterOnHitCB)m_onHitCallbacks[i])((Fighter*)targetTask, (StageObject*)attackerTask, damageDealt);
			}
		}
	}

	void registerFighterHooks()
	{
		// Hitbox Setup Hook @ 0x807468CC: 0x1EC bytes into symbol "notifyEventAnimCmd/[soCollisionAttackModuleImpl]/so_colli"
		//SyringeCore::syInlineHookRel(0x3BEB8, reinterpret_cast<void*>(setPartForDeflect), Modules::SORA_MELEE);

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

		// General Fighter Attack Land @ 0x8081A298: 0x3A4 bytes into symbol "notifyLogEventCollisionHit/[ftManager]/ft_manager_log_eve"
		SyringeCore::syInlineHookRel(0x10F884, reinterpret_cast<void*>(ftCallbackMgr::performOnAttackCallbacks), Modules::SORA_MELEE);

		// General Fighter Update Hook @ 0x80839160: 0xAA4 bytes into symbol "processUpdate/[Fighter]/fighter.o"
		SyringeCore::syInlineHookRel(0x12E74C, reinterpret_cast<void*>(ftCallbackMgr::performOnUpdateCallbacks), Modules::SORA_MELEE);
	}
}
