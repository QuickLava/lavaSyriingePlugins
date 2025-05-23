#ifndef LAVA_FIGHTER_HOOKS_H_V1
#define LAVA_FIGHTER_HOOKS_H_V1

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
#include <syWrapper.h>

namespace fighterHooks
{
	extern const char outputTag[];
	extern const char observerMessageFmt[];

	const u32 maxFighterCount = 0x8;

	typedef void (*GenericArglessCB)();
	typedef GenericArglessCB MeleeOnStartCB;
	typedef GenericArglessCB MeleeOnReadyGoCB;
	typedef GenericArglessCB MeleeOnUpdateCB;
	typedef GenericArglessCB MeleeOnGameSetCB;
	typedef void (*GenericFighterEventCB)(Fighter*);
	typedef GenericFighterEventCB FighterOnCreateCB;
	typedef GenericFighterEventCB FighterOnStartCB;
	typedef GenericFighterEventCB FighterOnRemoveCB;
	typedef GenericFighterEventCB FighterOnUpdateCB;
	typedef GenericFighterEventCB FighterOnStatusChangeCB;

	typedef void (*FighterOnHitCB)(Fighter*, StageObject*, float);
	typedef void (*FighterOnAttackCB)(Fighter*, StageObject*, float, StageObject*, u32, u32);

	enum attackKind
	{
		ak_NULL = 0x00,
		ak_ATTACK_1 = 0x01,
		ak_ATTACK_2 = 0x02,
		ak_ATTACK_3 = 0x03,
		ak_ATTACK_100 = 0x04,
		ak_ATTACK_DASH = 0x05,
		ak_ATTACK_S3_1 = 0x06,
		ak_ATTACK_S3_2 = 0x07,
		ak_ATTACK_S3_3 = 0x08,
		ak_ATTACK_Hi3 = 0x09,
		ak_ATTACK_Lw3 = 0x0A,
		ak_ATTACK_S4 = 0x0B,
		ak_ATTACK_Hi4 = 0x0C,
		ak_ATTACK_Lw4 = 0x0D,
	};
	enum attackSituation
	{
		as_NULL = 0x00,
		as_AttackerFighter,
		as_AttackerItem,
		as_AttackerWeapon,
	};
	struct callbackBundle
	{
		MeleeOnStartCB m_MeleeOnStartCB;
		MeleeOnReadyGoCB m_MeleeOnReadyGoCB;
		MeleeOnUpdateCB m_MeleeOnUpdateCB;
		MeleeOnGameSetCB m_MeleeOnGameSetCB;

		FighterOnCreateCB m_FighterOnCreateCB;
		FighterOnStartCB m_FighterOnStartCB;
		FighterOnRemoveCB m_FighterOnRemoveCB;
		FighterOnUpdateCB m_FighterOnUpdateCB;
		FighterOnStatusChangeCB m_FighterOnStatusChangeCB;

		FighterOnHitCB m_FighterOnHitCB;
		FighterOnAttackCB m_FighterOnAttackCB;
	};
	const u32 test = sizeof(callbackBundle);
#define CALLBACK_INDEX(callbackMember) offsetof(callbackBundle, callbackMember) / sizeof(void*)

#define INCLUDE_OUTSIDE_OBSERVER false

	u32 getFighterSlotNo(Fighter* fighterIn);
	u32 getFighterPlayerNo(Fighter* fighterIn);

	const u32 maxBundleCount = 0x20;
	class ftCallbackMgr
	{
	private:
#if INCLUDE_OUTSIDE_OBSERVER
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
		static void subscribeEventWatcher();
		static void unsubscribeEventWatcher();
#endif
		static u32 m_currBundleCount;
		static callbackBundle* m_callbackBundles[maxBundleCount];

		static void _performArglessCallbacks(u32 funcIndex);
		static void _performFighterEventCallbacks(u32 funcIndex, u32 entryID);

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
		static bool registerCallbackBundle(callbackBundle* bundleIn);
		static bool unregisterCallbackBundle(callbackBundle* bundleIn);

		// OnMeleeStart Callbacks
		static void performMeleeOnStartCallbacks();
		// OnMeleeReadyGo Callbacks
		static void performMeleeOnReadyGoCallbacks();
		// OnMeleeUpdate Callbacks
		static void performMeleeOnUpdateCallbacks();
		// OnMeleeGameSet Callbacks
		static void performMeleeOnGameSetCallbacks();

		// OnCreate Callbacks
		static void performOnCreateCallbacks();
		// OnStart Callbacks
		static void performOnStartCallbacks();
		// OnRemove Callbacks
		static void performOnRemoveCallbacks();
		// OnUpdate Callbacks
		static void performOnUpdateCallbacks();
		// OnStatusChange Callbacks
		static void performOnStatusChangeCallbacks();

		// OnHit & OnAttack Callbacks
		static void performOnAttackCallbacks();
	};
	void registerFighterHooks();
}

#endif
