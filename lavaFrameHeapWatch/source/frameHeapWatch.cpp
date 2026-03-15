#include <os/OSError.h>
#include <sy_compat.h>
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

    const char scEditStr[] = "scEdit";
    const char scIntroStr[] = "scIntro";
    const char scMeleeStr[] = "scMelee";
    const char scVsResultStr[] = "scVsResult";
    const char scClearGetterStr[] = "scClearGetter";
    const char scSelctCharacterStr[] = "scSelctCharacter";

    const char* toLoad[] = { scIntroStr, scVsResultStr, scSelctCharacterStr };
    const u32 toLoadLen = sizeof(toLoad) / sizeof(char*);
    const char* toUnload[] = { scEditStr, scMeleeStr, scClearGetterStr };
    const u32 toUnloadLen = sizeof(toUnload) / sizeof(char*);

    const char narrationMenuStr[] = "Narration_Menu";
    const char narrationMeleeStr[] = "Narration_Melee";
    const char narrationBankSwapMessage[] = "%sSwapped %s for %s in FH00!\n";
    const char selCharBankLoadUnloadMessage[] = "%sRequested %s for Narration_CharaCall & Select on entering %s!\n";

    u32 lowestRecordedFreeSize[0xC] = { 
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    };
    u32 highestRecordedFreeSize[0xC] = {
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
    };

    void recordInitialSizes()
    {
        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        for (u32 i = 0; i < 0xC; i++)
        {
            SoundHeap* currHeap = soundHeapArr + i;
            if (currHeap->m_frameHeap.m_heapPtr != NULL)
            {
                highestRecordedFreeSize[i] = currHeap->m_frameHeap.GetFreeSize();
            }
        }
    }
    void summarizeFrameHeaps()
    {
        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        for (u32 i = 0; i < 0xC; i++)
        {
            SoundHeap* currHeap = soundHeapArr + i;
            u32 currLevel = 0xFF;
            u32 freeSize = 0xFFFFFFFF;
            u32 lowestFreeSize = lowestRecordedFreeSize[i];
            u32 highestFreeSize = highestRecordedFreeSize[i];
            u32 freeSizeDiff = freeSize;
            if (currHeap->m_frameHeap.m_heapPtr != NULL)
            {
                currLevel = currHeap->m_frameHeap.GetCurrentLevel();
                freeSize = currHeap->m_frameHeap.GetFreeSize();
                if (lowestFreeSize > freeSize)
                {
                    lowestFreeSize = freeSize;
                    lowestRecordedFreeSize[i] = freeSize;
                }
                if (highestFreeSize < freeSize)
                {
                    highestFreeSize = freeSize;
                    highestRecordedFreeSize[i] = freeSize;
                }
            }
            OSReport_N("%sSummary: FrameHeap[%02X]: Free: %08X, MinFree: %08X, MaxFree: %08X, CurrLevel: %02X\n",
                outputTag, i, freeSize, lowestFreeSize, highestFreeSize, currLevel);
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
        u32 freeSpace = frameHeap->GetFreeSize();
        u32 current = frameHeap->GetCurrentLevel();

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
        u32 freeSize = frameHeap->GetFreeSize();

        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        u32 soundHeapID = soundHeap - soundHeapArr;
        if (lowestRecordedFreeSize[soundHeapID] > freeSize)
        {
            lowestRecordedFreeSize[soundHeapID] = freeSize;
        }

        OSReport_N("%sSaveState: FrameHeap[%02X] Raised to Level: %02X, FreeSize: %08X\n",
            outputTag, soundHeapID, frameHeap->GetCurrentLevel(), freeSize);
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
            outputTag, soundHeapID, soundHeap->m_frameHeap.GetCurrentLevel(), soundHeap->m_frameHeap.GetFreeSize());
        summarizeFrameHeaps();
    }

    enum heapLevelIndex
    {
        hli_Select = 0x00,
        hli_Narration_Menu,
        hli_Narration_Melee,
        hli_Narration_CharaCall,
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
            case Snd_Group_Select: { backupIndex = hli_Select; break; }
            case Snd_Group_Narration_Menu: { backupIndex = hli_Narration_Menu; break; }
            case Snd_Group_Narration_Melee: { backupIndex = hli_Narration_Melee; break; }
            case Snd_Group_Narration_CharaCall: { backupIndex = hli_Narration_CharaCall; break; }
            default: { return; }
        }
        heapLevelBackupArr[backupIndex] = determinedHeapLevel;
        OSReport_N("%sBacked Up Group %02X: HLI = %08X\n", outputTag, groupID, determinedHeapLevel);
    }

    enum sceneCacheIndex
    {
        sci_PrevPrev = 0x00,
        sci_Prev,
        sci_Curr,
        sci_Next,
    };
    class sceneCache
    {
    private:
        u8 currIndex;
        gfScene* scenes[4];
    public:        
        void pushNextScene(gfScene* sceneIn)
        {
            u32 targetIndex = currIndex;
            scenes[targetIndex] = sceneIn;
            currIndex = (targetIndex + 1) % 4;
        }
        gfScene* getScene(u32 index)
        {
            u32 targetIndex = (currIndex + index) % 4;
            return scenes[targetIndex];
        }
        void print()
        {
            OSReport_N("%s", outputTag);

            const char nullStr[] = "-";
            gfSequence* currentSequence = gfSceneManager::getInstance()->m_currentSequence;
            OSReport_N("%s: ", (currentSequence == NULL) ? nullStr : currentSequence->m_sequenceName);
            gfScene* targetScene;
            for (u32 i = sci_PrevPrev; i < sci_Next; i++)
            {
                targetScene = getScene(i);
                OSReport_N("%s -> ", (targetScene == NULL) ? nullStr : targetScene->m_sceneName);
            }
            targetScene = getScene(sci_Next);
            OSReport_N("%s\n", (targetScene == NULL) ? nullStr : targetScene->m_sceneName);
        }
    };

    void onSceneChange()
    {
        static sceneCache s_sceneCache;

        gfSceneManager* sceneManager = gfSceneManager::getInstance();
        s_sceneCache.pushNextScene(sceneManager->m_nextScene);

        gfScene* prevScene = s_sceneCache.getScene(sci_Prev);
        gfScene* nextScene = s_sceneCache.getScene(sci_Next);
        s_sceneCache.print();
        if (prevScene != NULL && nextScene != NULL)
        {
            sndSystem* soundSys = g_sndSystem;
            // If we're moving into scMelee...
            if (strcmp(nextScene->m_sceneName, scMeleeStr) == 0)
            {
                // ... swap out the Menu Narration bank.
                soundSys->freeGroup(heapLevelBackupArr[hli_Narration_Menu], 0);
                soundSys->loadSoundGroup(Snd_Group_Narration_Melee, 0x0, 1);
                OSReport_N(narrationBankSwapMessage, outputTag, narrationMenuStr, narrationMeleeStr);
            }
            // If we're not moving into scMelee and instead are moving *out of* scMelee...
            else if (strcmp(prevScene->m_sceneName, scMeleeStr) == 0)
            {
                // ... swap out the Melee Narration bank.
                soundSys->freeGroup(heapLevelBackupArr[hli_Narration_Melee], 0);
                soundSys->loadSoundGroup(Snd_Group_Narration_Menu, 0x0, 1);
                OSReport_N(narrationBankSwapMessage, outputTag, narrationMeleeStr, narrationMenuStr);
            }

            for (u32 i = 0; i < toUnloadLen; i++)
            {
                const char* triggerSceneName = toUnload[i];
                if (strcmp(nextScene->m_sceneName, triggerSceneName) == 0)
                {
                    // Unload SelChar banks.
                    soundSys->freeGroup(heapLevelBackupArr[hli_Select], 0);
                    soundSys->freeGroup(heapLevelBackupArr[hli_Narration_CharaCall], 0);
                    OSReport_N(selCharBankLoadUnloadMessage, outputTag, "Unload", triggerSceneName);
                    break;
                }
            }
            for (u32 i = 0; i < toLoadLen; i++)
            {
                const char* triggerSceneName = toLoad[i];
                if (strcmp(nextScene->m_sceneName, triggerSceneName) == 0)
                {
                    // Load SelChar banks.
                    soundSys->loadSoundGroup(Snd_Group_Narration_CharaCall, 0x1, 1);
                    soundSys->loadSoundGroup(Snd_Group_Select, 0x1, 1);
                    OSReport_N(selCharBankLoadUnloadMessage, outputTag, "Load", triggerSceneName);
                    break;
                }
            }
        }
    }

    // Patches the common bank loading routine in loadFiles2 to set up for our contextual loading scheme.
    // Specifically, the new arrangement is now:
    //  - FH00:
    //    - Joucyu
    //    - Menu
    //    - Narration Menu / Narration_Melee
    //  - FH01 (Out of Matches Only):
    //    - Select
    //    - CharaCall
    //  - FH02:
    //    - Item
    //    - Monsterball
    void applyHeapPatches(gfModuleInfo* loadedModuleIn)
    {
        gfModuleHeader* moduleHeader = loadedModuleIn->m_module->header;
        if (moduleHeader->id == Modules::SORA_SCENE)
        {
            // SoraScene = 0x806BB554
            u32 textAddr = moduleHeader->getTextSectionAddr();
            *(u32*)(textAddr + 0x437C) = 0x60000000; // op nop         @ $806BF8D0, Disable Narration_Melee load, as we'll only be loading it in matches.
            *(u32*)(textAddr + 0x43D8) = 0x38A00001; // op li r5, 0x01 @ $806BF92C, Load Narration_CharaCall in FH1
            *(u32*)(textAddr + 0x43EC) = 0x38A00001; // op li r5, 0x01 @ $806BF940, Load Select in FH1
            OSReport_N("%sApplied bank loading adjustments!\n", outputTag);
            return;
        }
    }

    void Init()
    {
        // Record Initial Heap Sizes
        recordInitialSizes();

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

        // 0x8002D628 lands 0x7C bytes into symbol "setNextScene/[gfSceneManager]/gf_scene.o" @ 0x8002D5AC
        SyringeCompat::syInlineHook(0x8002D628, reinterpret_cast<void*>(onSceneChange));
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}