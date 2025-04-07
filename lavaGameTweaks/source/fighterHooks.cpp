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

	u32 ftCallbackMgr::m_currBundleCount = 0x00;
	callbackBundle* ftCallbackMgr::m_callbackBundles[maxBundleCount];

	void ftCallbackMgr::_performArglessCallbacks(u32 funcIndex)
	{
		for (u32 i = 0; i < m_currBundleCount; i++)
		{
			callbackBundle* targetBundle = m_callbackBundles[i];
			GenericArglessCB currCallback = ((GenericArglessCB*)targetBundle)[funcIndex];
			if (currCallback != NULL)
			{
				currCallback();
			}
		}
	}
	void ftCallbackMgr::_performFighterEventCallbacks(u32 funcIndex, u32 fighterEntryID)
	{
		// Execute Callbacks
		Fighter* fighterIn = g_ftManager->getFighter(fighterEntryID, -1);
		for (u32 i = 0; i < m_currBundleCount; i++)
		{
			callbackBundle* targetBundle = m_callbackBundles[i];
			GenericFighterEventCB currCallback = ((GenericFighterEventCB*)targetBundle)[funcIndex];
			if (currCallback != NULL)
			{
				currCallback(fighterIn);
			}
		}
	}

	bool ftCallbackMgr::registerCallbackBundle(callbackBundle* bundleIn)
	{
		bool result = m_currBundleCount < maxBundleCount;

		for (int i = 0; result && i < m_currBundleCount; i++)
		{
			result = m_callbackBundles[i] != bundleIn;
		}

		if (result)
		{
			m_callbackBundles[m_currBundleCount] = bundleIn;
			m_currBundleCount++;
		}

		return result;
	}
	bool ftCallbackMgr::unregisterCallbackBundle(callbackBundle* bundleIn)
	{
		bool result = 0;

		for (int i = 0; !result && i < m_currBundleCount; i++)
		{
			if (result || m_callbackBundles[i] == bundleIn && ((i + 1) < m_currBundleCount))
			{
				result = 1;
				m_callbackBundles[i] = m_callbackBundles[i + 1];
			}
		}

		if (result)
		{
			m_currBundleCount--;
		}

		return result;
	}
	
	// MeleeOnStart Callbacks
	void ftCallbackMgr::performMeleeOnStartCallbacks()
	{
		OSReport_N("%sOnMeleeStart Callbacks\n", outputTag);

		// Execute Callbacks
		_performArglessCallbacks(CALLBACK_INDEX(callbackBundle::m_MeleeOnStartCB));

#if INCLUDE_OUTSIDE_OBSERVER
		// Subscribe Event Watcher
		subscribeEventWatcher();
#endif
	}
	// MeleeOnReadyGo Callbacks
	void ftCallbackMgr::performMeleeOnReadyGoCallbacks()
	{
		OSReport_N("%sOnMeleeReadyGo Callbacks\n", outputTag);

		// Execute Callbacks
		_performArglessCallbacks(CALLBACK_INDEX(callbackBundle::m_MeleeOnReadyGoCB));
	}
	// MeleeOnGameSet Callbacks
	void ftCallbackMgr::performMeleeOnGameSetCallbacks()
	{
		OSReport_N("%sOnMeleeGameSet Callbacks\n", outputTag);

		// Execute Callbacks
		_performArglessCallbacks(CALLBACK_INDEX(callbackBundle::m_MeleeOnGameSetCB));

#if INCLUDE_OUTSIDE_OBSERVER
		// Unsubscribe Event Watcher
		unsubscribeEventWatcher();
#endif
	}

	// OnCreate Callbacks
	void ftCallbackMgr::performOnCreateCallbacks()
	{
		register u32 entryID;
		asm
		{
			lwz entryID, 0x04(r31);
		}

		OSReport_N(callbackHookMsgFmt, outputTag, "OnCreate Callbacks", entryID, g_ftManager->getPlayerNo(entryID));

		// Execute Callbacks
		_performFighterEventCallbacks(CALLBACK_INDEX(callbackBundle::m_FighterOnCreateCB), entryID);
	}
	// OnStart Callbacks
	void ftCallbackMgr::performOnStartCallbacks()
	{
		register u32 entryID;
		asm
		{
			mr entryID, r4;
		}

		OSReport_N(callbackHookMsgFmt, outputTag, "OnStart Callbacks", entryID, g_ftManager->getPlayerNo(entryID));

		// Execute Callbacks
		_performFighterEventCallbacks(CALLBACK_INDEX(callbackBundle::m_FighterOnStartCB), entryID);
	}
	// OnRemove Callbacks
	void ftCallbackMgr::performOnRemoveCallbacks()
	{
		register u32 entryID;
		asm
		{
			mr entryID, r4;
		}

		OSReport_N(callbackHookMsgFmt, outputTag, "OnRemove Callbacks", entryID, g_ftManager->getPlayerNo(entryID));

		// Execute Callbacks
		_performFighterEventCallbacks(CALLBACK_INDEX(callbackBundle::m_FighterOnRemoveCB), entryID);
	}
	// OnUpdate Callbacks
	void ftCallbackMgr::performOnUpdateCallbacks()
	{
		register Fighter* fighter;
		asm
		{
			mr fighter, r29
		}
		sizeof(fighter);

		// Execute Callbacks
		_performFighterEventCallbacks(CALLBACK_INDEX(callbackBundle::m_FighterOnUpdateCB), fighter->m_entryId);
	}
	// OnStatusChange Callbacks
	void ftCallbackMgr::performOnStatusChangeCallbacks()
	{
		register StageObject* stageObj;
		asm
		{
			lwz stageObj, 0x08(r31)
		}
		sizeof(stageObj);

		if (stageObj->m_taskCategory == gfTask::Category_Fighter)
		{
			// Execute Callbacks
			_performFighterEventCallbacks(CALLBACK_INDEX(callbackBundle::m_FighterOnStatusChangeCB), ((Fighter*)stageObj)->m_entryId);
		}
	}

	// OnHit & OnAttack Callbacks
	void ftCallbackMgr::performOnAttackCallbacks()
	{
		const char fmtStr1[] = "%s- %8s: %s, TaskID: 0x%08X, Category: 0x%04X\n";

		register gfTask* attackerTask;
		register gfTask* attackerParentTask;
		register gfTask* targetTask;
		register u32 attackKind;
		register float damageDealt;

		asm
		{
			mr attackerTask, r31;
			mr attackerParentTask, r30;
			mr targetTask, r29;
			mr attackKind, r25;
			fmr damageDealt, f31;
		}

		OSReport_N("%sOnAttack & OnHit Callbacks: ???: 0x%02X, Damage: %2.0f!\n", outputTag, attackKind, damageDealt);

		gfTask::Category attackerCategory = attackerTask->m_taskCategory;
		gfTask::Category targetCategory = targetTask->m_taskCategory;
		OSReport_N(fmtStr1, outputTag, "Attacker", attackerTask->m_taskName, attackerTask->m_taskId, attackerCategory);
		if (attackerParentTask != NULL && attackerParentTask != attackerTask)
		{
			OSReport_N(fmtStr1, outputTag, "Parent", attackerParentTask->m_taskName, attackerParentTask->m_taskId, attackerParentTask->m_taskCategory);
		}
		if (targetTask != NULL)
		{
			OSReport_N(fmtStr1, outputTag, "Target", targetTask->m_taskName, targetTask->m_taskId, targetTask->m_taskCategory);
		}
		else
		{
			OSReport_N("%s-   Target: NULL\n");
		}

		attackSituation situation = as_NULL;
		if (attackerCategory == gfTask::Category_Fighter)
		{
			situation = as_AttackerFighter;
		}
		else if (attackerParentTask != NULL && attackerParentTask->m_taskCategory == gfTask::Category_Fighter)
		{
			gfTask* temp = attackerTask;
			attackerTask = attackerParentTask;
			attackerParentTask = temp;
			if (attackerCategory == gfTask::Category_Item)
			{
				situation = as_AttackerItem;
			}
			else if (attackerCategory == gfTask::Category_Weapon)
			{
				situation = as_AttackerWeapon;
			}
		}
		if (situation != as_NULL)
		{
			int bundleCount = m_currBundleCount;
			for (int i = 0; i < bundleCount; i++)
			{
				callbackBundle* currentBundle = m_callbackBundles[i];
				FighterOnAttackCB attackCB = currentBundle->m_FighterOnAttackCB;
				if (attackCB != NULL)
				{
					attackCB((Fighter*)attackerTask, (StageObject*)targetTask, damageDealt, (StageObject*)attackerParentTask, attackKind, situation);
				}
				FighterOnHitCB hitCB = currentBundle->m_FighterOnHitCB;
				if (hitCB != NULL)
				{
					hitCB((Fighter*)targetTask, (StageObject*)attackerTask, damageDealt);
				}
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

		// General Fighter Status Change Hook @ 0x8077FE64: 0x4A0 bytes into symbol "changeStatus/[soStatusModuleImpl]/so_status_module_impl.o"
		SyringeCore::syInlineHookRel(0x75450, reinterpret_cast<void*>(ftCallbackMgr::performOnStatusChangeCallbacks), Modules::SORA_MELEE);
	}
}
