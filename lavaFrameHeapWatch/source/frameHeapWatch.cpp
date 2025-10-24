#include <os/OSError.h>
#include <syWrapper.h>
#include <OS/OSCache.h>
#include <revolution/FA.h>
#include <gf/gf_scene.h>
#include <gf/gf_heap_manager.h>
#include <memory.h>
#include <modules.h>
#include <string.h>
#include <logUtils.h>
#include <snd/snd_id.h>
#include <snd/snd_system.h>

namespace lavaFrameHeapWatch {

    const char outputTag[] = "[frameHeapWatch] ";

    extern u32 GetFreeSize(FrameHeap* frameHeap);
    extern u32 GetCurrentLevel(FrameHeap* frameHeap);
    extern u32 MEMGetAllocatableSizeForFrmHeapEx(FrameHeap* heap, u32 padding);

    u32 lowestRecordedFreeSize[0xC] = { 
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    };

    void summarizeFrameHeaps()
    {
        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        for (u32 i = 0; i < 0xC; i++)
        {
            SoundHeap* currHeap = soundHeapArr + i;
            u32 currLevel = 0xFF;
            u32 freeSize = 0xFFFFFFFF;
            u32 lowestFreeSize = lowestRecordedFreeSize[i];
            u32 freeSizeDiff = freeSize;
            if (currHeap->m_frameHeap.m_heapPtr != NULL)
            {
                currLevel = GetCurrentLevel(&currHeap->m_frameHeap);
                freeSize = GetFreeSize(&currHeap->m_frameHeap);
                if (lowestFreeSize > freeSize)
                {
                    lowestFreeSize = freeSize;
                    lowestRecordedFreeSize[i] = freeSize;
                }
            }
            OSReport_N("%sSummary: FrameHeap[%02X]: FreeSize: %08X, MinFreeSize: %08X, CurrLevel: %02X\n",
                outputTag, i, freeSize, lowestFreeSize, currLevel);
        }
    }
    void printFrameHeapAlloc()
    {
        register u32* destination;
        register FrameHeap* frameHeap;
        register SoundHeap* soundHeap;
        register u32 size;
        asm
        {
            mr destination, r3;
            mr frameHeap, r27;
            subi soundHeap, frameHeap, 0x1C;
            mr size, r28;
        }
        u32 freeSpace = GetFreeSize(frameHeap);
        u32 current = GetCurrentLevel(frameHeap);

        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        u32 soundHeapID = soundHeap - soundHeapArr;

        if (lowestRecordedFreeSize[soundHeapID] > freeSpace)
        {
            lowestRecordedFreeSize[soundHeapID] = freeSpace;
        }

        OSReport_N("%sAlloc: FrameHeap[%02X] @ %08X, Size: %08X (FreeSize: %08X, CurrLevel: %02X)\n", outputTag, soundHeapID, destination, size, freeSpace, current);
    }

    void printFrameSaveState()
    {
        register FrameHeap* frameHeap;
        register SoundHeap* soundHeap;
        asm
        {
            mr frameHeap, r31;
            subi soundHeap, frameHeap, 0x1C;
        }
        u32 freeSize = GetFreeSize(frameHeap);

        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        u32 soundHeapID = soundHeap - soundHeapArr;
        if (lowestRecordedFreeSize[soundHeapID] > freeSize)
        {
            lowestRecordedFreeSize[soundHeapID] = freeSize;
        }

        OSReport_N("%sSaveState: FrameHeap[%02X] Raised to Level: %02X, FreeSize: %08X\n",
            outputTag, soundHeapID, GetCurrentLevel(frameHeap), freeSize);
        summarizeFrameHeaps();
    }
    void printFrameLoadState()
    {
        register SoundHeap* soundHeap;
        register u32 targetHeapLevel;
        asm
        {
            mr soundHeap, r29;
            mr targetHeapLevel, r30;
        }
        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        u32 soundHeapID = soundHeap - soundHeapArr;

        OSReport_N("%sLoadState: FrameHeap[%02X] Cleared to Level: %02X, FreeSize: %08X\n", 
            outputTag, soundHeapID, GetCurrentLevel(&soundHeap->m_frameHeap), GetFreeSize(&soundHeap->m_frameHeap));
        summarizeFrameHeaps();
    }

    enum heapLevelIndex
    {
        hli_Menu = 0x00,
        hli_Select,
        hli_PokeTrainer,
        hli_Narration_Menu,
        hli_Narration_Melee,
        hli__COUNT,
    };
    u32 heapLevelBackupArr[hli__COUNT];
    void backupGroupHeapID()
    {
        register u32 groupID;
        register u32 determinedHeapLevel;
        asm
        {
            mr groupID, r28;
            mr determinedHeapLevel, r31;
        }
        u32 backupIndex;
        switch (groupID)
        {
            case Snd_Group_Menu: { backupIndex = hli_Menu; break; }
            case Snd_Group_Select: { backupIndex = hli_Select; break; }
            case Snd_Group_Fighter_PokeTrainer: { backupIndex = hli_PokeTrainer; break; }
            case Snd_Group_Narration_Menu: { backupIndex = hli_Narration_Menu; break; }
            case Snd_Group_Narration_Melee: { backupIndex = hli_Narration_Melee; break; }
            default: { return; }
        }
        heapLevelBackupArr[backupIndex] = determinedHeapLevel;
        OSReport_N("%sBacked Up Group %02X: HLI = %08X\n", outputTag, groupID, determinedHeapLevel);
    }
    void onMeleeStart()
    {
        sndSystem* soundSys = g_sndSystem;
        soundSys->freeGroup(heapLevelBackupArr[hli_Menu], 0);
        soundSys->loadSoundGroup(Snd_Group_Narration_Melee, 0x0, 0);
        OSReport_N("%sMeleeStart: Swapped Menu for Narration_Melee!\n", outputTag);
        soundSys->freeGroup(heapLevelBackupArr[hli_Select], 0);
        soundSys->freeGroup(heapLevelBackupArr[hli_Narration_Menu], 0);
        OSReport_N("%sMeleeStart: Unloaded Select and Narration_Menu!\n", outputTag);
    }
    void onMeleeExit()
    {
        sndSystem* soundSys = g_sndSystem;
        soundSys->freeGroup(heapLevelBackupArr[hli_Narration_Melee], 0);
        soundSys->loadSoundGroup(Snd_Group_Menu, 0x0, 1);
        OSReport_N("%sMeleeExit: Swapped Narration_Melee for Menu!\n", outputTag);
        soundSys->loadSoundGroup(Snd_Group_Select, 0x1, 1);
        soundSys->loadSoundGroup(Snd_Group_Narration_Menu, 0x1, 1);
        OSReport_N("%sMeleeExit: Loaded Select and Narration_Menu!\n", outputTag);
    }

    void applyHeapPatches(gfModuleInfo* loadedModuleIn)
    {
        gfModuleHeader* moduleHeader = loadedModuleIn->m_module->header;
        if (moduleHeader->id == Modules::SORA_SCENE)
        {
            u32 textAddr = moduleHeader->getTextSectionAddr();
            *(u32*)(textAddr + 0x4370) = 0x38800000; // op li r4, 0x00 @ $806BF8C4, Load Joucyu instead of Narration_Melee in FH0 Level 0
            *(u32*)(textAddr + 0x4398) = 0x388000D8; // op li r4, 0xD8 @ $806BF8EC, Load CharCall instead of Joucyu in FH0 Level 1
            *(u32*)(textAddr + 0x43C4) = 0x38A00001; // op li r5, 0x01 @ $806BF918, Load Narration_Menu in FH1
            *(u32*)(textAddr + 0x43E0) = 0x60000000; // op nop         @ $806BF934, Disable original CharCall load (normally would load Narration_Melee here)
            *(u32*)(textAddr + 0x43EC) = 0x38A00001; // op li r5, 0x01 @ $806BF940, Load Select in FH1
            OSReport_N("%sApplied bank loading adjustments!\n", outputTag);
            return;
        }
    }

    void Init()
    {
        // Patch bank loading configuration.
        SyringeCompat::ModuleLoadEvent::Subscribe(applyHeapPatches);

        // 0x801BFBDC lands 0x54 bytes into symbol "Alloc/[nw4r3snd6detail9FrameHeapFUlPFPvUlPv_vPv]/snd_Fram" @ 0x801BFB88
        SyringeCompat::syInlineHook(0x801BFBDC, reinterpret_cast<void*>(printFrameHeapAlloc));
        // 0x801BFCBC lands 0x90 bytes into symbol "SaveState/[nw4r3snd6detail9FrameHeapFv]/snd_FrameHeap.o" @ 0x801BFC2C
        SyringeCompat::syInlineHook(0x801BFCBC, reinterpret_cast<void*>(printFrameSaveState));
        // 0x801CAA80 lands 0x58 bytes into symbol "LoadState/[nw4r3snd9SoundHeapFi]/snd_SoundHeap.o" @ 0x801CAA28
        SyringeCompat::syInlineHook(0x801CAA80, reinterpret_cast<void*>(printFrameLoadState));

        // 0x80073C1C lands 0x35C bytes into symbol "update/[sndSystem]/snd_system.o" @ 0x800738C0
        SyringeCompat::syInlineHook(0x80073C1C, reinterpret_cast<void*>(backupGroupHeapID));

        // SoraScene = 0x806BB554
        // 0x806CF160 lands 0x20 bytes into symbol "start/[scMelee]/sc_melee.o" @ 0x806CF140
        SyringeCompat::syInlineHookRel(0x13C0C, reinterpret_cast<void*>(onMeleeStart), Modules::SORA_SCENE);
        // 0x806D4870 lands 0x34 bytes into symbol "exit/[scMelee]/sc_melee.o" @ 0x806D483C 
        SyringeCompat::syInlineHookRel(0x1931C, reinterpret_cast<void*>(onMeleeExit), Modules::SORA_SCENE);
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}