#include "fighterHooks.h"

namespace fighterHooks
{
	const char outputTag[] = "[fighterHooks] ";

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

	ftCallbackMgr::ftAttackWatcher ftCallbackMgr::m_attackWatcher;
	Vector<FighterUpdateCB> ftCallbackMgr::m_updateCallbacks;
	Vector<FighterOnStartCB> ftCallbackMgr::m_onStartCallbacks;
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

		OSReport("%sOnStart Callbacks: entryID = 0x%08X!\n", outputTag, entryID);

		Fighter* fighter = manager->getFighter(entryID, -1);

		// Subscribe Watchers
		subscribeWatcherToFighter(m_attackWatcher, fighter);

		// Execute Callbacks
		for (int i = 0; i < m_onStartCallbacks.size(); i++)
		{
			m_onStartCallbacks[i](fighter);
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

		// General Fighter Start Hook @ 0x80814774: 0x10 bytes into symbol "startFighter/[ftManager]/ft_manager.o"
		SyringeCore::syInlineHookRel(0x109D60, reinterpret_cast<void*>(ftCallbackMgr::performOnStartCallbacks), Modules::SORA_MELEE);
	}
}
