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
#include <nw4r/snd/snd_WsdFile.h>
#include <FA/FAFstat.h>

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

    void summarizeFrameHeaps()
    {
        using namespace nw4r::snd;
        sndHeapSys* heapSys = g_sndSystem->m_sndHeapSys;
        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        for (u32 i = 0; i < 0xC; i++)
        {
            SoundHeap* currHeap = soundHeapArr + i;
            u32 currLevel = 0xFF;
            u32 freeSize = 0xFFFFFFFF;
            u32 lowestFreeSize = lowestRecordedFreeSize[i];
            u32 maxFreeSize = heapSys->m_heapMaxSizeArr[i];
            u32 freeSizeDiff = freeSize;
            if (currHeap->IsValid() != NULL)
            {
                currLevel = currHeap->GetCurrentLevel();
                freeSize = currHeap->GetFreeSize();
                if (lowestFreeSize > freeSize)
                {
                    lowestFreeSize = freeSize;
                    lowestRecordedFreeSize[i] = freeSize;
                }
            }
            OSReport_N("%sSummary: FrameHeap[%02X]: Free: %08X, MinFree: %08X, MaxFree: %08X, CurrLevel: %02X\n",
                outputTag, i, freeSize, lowestFreeSize, maxFreeSize, currLevel);
        }
    }
    void printFrameHeapAlloc()
    {
        using namespace nw4r::snd;
        register u32* destination;
        register SoundHeap* soundHeap;
        register u32 size;
        asm
        {
            mr destination, r3;
            subi soundHeap, r27, 0x1C;
            mr size, r28;
        }
        u32 freeSpace = soundHeap->GetFreeSize();
        u32 current = soundHeap->GetCurrentLevel();

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
        using namespace nw4r::snd;
        register SoundHeap* soundHeap;
        asm
        {
            subi soundHeap, r31, 0x1C;
        }
        u32 freeSize = soundHeap->GetFreeSize();

        SoundHeap* soundHeapArr = g_sndSystem->m_sndHeapSys->m_heapArr;
        u32 soundHeapID = soundHeap - soundHeapArr;
        if (lowestRecordedFreeSize[soundHeapID] > freeSize)
        {
            lowestRecordedFreeSize[soundHeapID] = freeSize;
        }

        OSReport_N("%sSaveState: FrameHeap[%02X] Raised to Level: %02X, FreeSize: %08X\n",
            outputTag, soundHeapID, soundHeap->GetCurrentLevel(), freeSize);
        summarizeFrameHeaps();
    }
    void printFrameLoadState()
    {
        using namespace nw4r::snd;
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
            outputTag, soundHeapID, soundHeap->GetCurrentLevel(), soundHeap->GetFreeSize());
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
        }
    }

    u32 (*_loadSoundGroup)(sndSystem*, u32, u32, u32);
    u32 loadSoundGroup(sndSystem* sndSystem, u32 groupIndex, u32 heapID, u32 isRequest)
    {
        using namespace nw4r::snd::detail;
        char pathBuf[0x60];

        if (!sndSystem->isSoundGroupLoaded(static_cast<SndGroupID>(groupIndex)))
        {
            const SoundArchiveFile::Info* infoSection = sndSystem->m_dvdSoundArchive.mFileReader.mInfo;
            SoundArchiveFile::GroupTable* groupArr = Util::GetDataRefAddress0(infoSection->groupTableRef, infoSection);

            SoundArchiveFile::GroupInfo* targetGroup = Util::GetDataRefAddress0(groupArr->items[(groupIndex < groupArr->count) ? groupIndex : (groupArr->count - 1)], infoSection);

            sprintf(pathBuf, "%spf/sfx/%03X.sawnd", MOD_PATCH_DIR, groupIndex);
            FAHandle* sawndStrmHandle = FAFopen(pathBuf, "r");
            if (sawndStrmHandle != NULL)
            {
                FAStat fileStats; FAFstat(pathBuf, &fileStats);
                OSReport_N("%s\"%s\" successfully loaded!\n", outputTag, pathBuf);
                FAFread(pathBuf, 1, 1, sawndStrmHandle);
                if (*pathBuf == 0x2)
                {
                    FAFread(pathBuf, 1, 0x8, sawndStrmHandle);
                    u32 temp = *(u32*)pathBuf;
                    if ((temp - 0x7) == groupIndex)
                    {
                        OSReport_N("%sInitial Header: Header Off: %08X, Size: %06X, Data Off: %08X, Size: %06X\n", outputTag,
                            targetGroup->offset, targetGroup->size, targetGroup->waveDataOffset, targetGroup->waveDataSize);
                        temp = ((u32*)pathBuf)[1];
                        targetGroup->waveDataSize = temp;

                        SoundArchiveFile::GroupItemTable* entryArr = Util::GetDataRefAddress0(targetGroup->itemTableRef, infoSection);
                        u32 fileCount = entryArr->count;
                        for (u32 i = 0x0; i < fileCount; i++)
                        {
                            SoundArchiveFile::GroupItemInfo* currEntry = Util::GetDataRefAddress0(entryArr->items[i], infoSection);
                            OSReport_N("%s  Initial File 0x%03X:  H.Off: %08X, Size: %06X, D.Off: %08X, Size: %06X\n", outputTag, currEntry->fileId,
                                currEntry->offset, currEntry->size, currEntry->waveDataOffset, currEntry->waveDataSize);
                            FAFread(pathBuf, 1, 0x4, sawndStrmHandle);
                            temp = ((u32*)pathBuf)[0];
                            if (temp != currEntry->fileId)
                            {
                                OSReport_N("%s  Warning: Incoming File ID (0x%03X) != Stored File ID (0x%03X)!\n", outputTag, temp, currEntry->fileId);
                            }
                            FAFread(&currEntry->waveDataOffset, 1, 0x8, sawndStrmHandle);
                        }
                        u32 headerLengthAcc = 0x00;
                        for (u32 i = 0x0; i < fileCount; i++)
                        {
                            SoundArchiveFile::GroupItemInfo* currEntry = Util::GetDataRefAddress0(entryArr->items[i], infoSection);
                            currEntry->offset = headerLengthAcc;
                            FAFread(pathBuf, 1, 0x10, sawndStrmHandle);
                            temp = ((u32*)pathBuf)[2];
                            currEntry->size = temp;
                            FAFseek(sawndStrmHandle, temp - 0x10, 1);
                            headerLengthAcc += temp;
                            OSReport_N("%s  Patched File 0x%03X:  H.Off: %08X, Size: %06X, D.Off: %08X, Size: %06X\n", outputTag, currEntry->fileId,
                                currEntry->offset, currEntry->size, currEntry->waveDataOffset, currEntry->waveDataSize);
                        }
                        targetGroup->size = headerLengthAcc;
                        targetGroup->waveDataOffset = targetGroup->offset + headerLengthAcc;
                        OSReport_N("%sPatched Header: Header Off: %08X, Size: %06X, Data Off: %08X, Size: %06X\n", outputTag,
                            targetGroup->offset, targetGroup->size, targetGroup->waveDataOffset, targetGroup->waveDataSize);
                    }
                }
                FAFclose(sawndStrmHandle);
            }
        }

        return _loadSoundGroup(sndSystem, groupIndex, heapID, isRequest);
    }

    u32(*_ReadSoundInfo)(nw4r::snd::detail::SoundArchiveFileReader*, u32, nw4r::snd::SoundArchive::SoundInfo*);
    u32 ReadSoundInfo(nw4r::snd::detail::SoundArchiveFileReader* reader, u32 soundID, nw4r::snd::SoundArchive::SoundInfo* soundInfoOut)
    {
        using namespace nw4r::snd;
        using namespace nw4r::snd::detail;
        u32 result = _ReadSoundInfo(reader, soundID, soundInfoOut);
        if (result && reader->GetSoundType(soundID) == nw4r::snd::SOUND_TYPE_WAVE)
        {
            SoundArchive::WaveSoundInfo waveSoundInfo;
            if (reader->ReadWaveSoundInfo(soundID, &waveSoundInfo))
            {
                WaveSoundNoteInfo noteData;
                WsdFileReader rwsdReader(g_sndSystem->m_archivePlayer.detail_GetFileAddress(soundInfoOut->fileId));
                if (rwsdReader.ReadWaveSoundNoteInfo(&noteData, waveSoundInfo.subNo, 0) && noteData.volume != 0x7F)
                {
                    OSReport_N("%sOverwrote Volume! Sound 0x%04X, %02X -> %02X! \n", outputTag, soundID, soundInfoOut->volume, noteData.volume);
                    soundInfoOut->volume = noteData.volume;
                }
            }
        }
        return result;
    }

    void reportFailedLoad()
    {
        register u32 heapID;
        register u32 groupID;
        register u32 groupSize;
        register sndHeapSys* heapSys;
        asm
        {
            mr heapID, r31;
            mr groupID, r27;
            mr groupSize, r30;
            mr heapSys, r29;
        }

        u32 heapMaxSize = heapSys->m_heapMaxSizeArr[heapID];
        u32 heapCurrSize = heapSys->m_heapCurrSizeArr[heapID];
        if ((groupSize + heapCurrSize) > heapMaxSize)
        {
            OSReport_N("%sGroup Load Failed: %03X (%06X bytes) -> FH%02X (%X of %X used)\n", outputTag, groupID, groupSize, heapID, heapCurrSize, heapMaxSize);
        }
    }

    void Init()
    {
        // Patch bank loading configuration.
        SyringeCompat::ModuleLoadEvent::Subscribe(applyHeapPatches);

        // 0x80073C34 lands 0x374 bytes into symbol "update/[sndSystem]/snd_system.o" @ 0x800738C0
        SyringeCompat::syInlineHook(0x80073C34, reinterpret_cast<void*>(backupGroupHeapID));
        // 0x80073C4C lands 0x38C bytes into symbol "update/[sndSystem]/snd_system.o" @ 0x800738C0
        SyringeCompat::syInlineHook(0x80073C4C, reinterpret_cast<void*>(backupGroupHeapID));
        // 0x8002D628 lands 0x7C bytes into symbol "setNextScene/[gfSceneManager]/gf_scene.o" @ 0x8002D5AC
        SyringeCompat::syInlineHook(0x8002D628, reinterpret_cast<void*>(onSceneChange));
        // 0x80073B68 lands 0x00 bytes into symbol "sndSystem::loadSoundGroup" @ 0x80073B68
        SyringeCompat::syReplaceFunc(0x80073B68, reinterpret_cast<void*>(loadSoundGroup), reinterpret_cast<void**>(&_loadSoundGroup));
        // 0x801C7384 lands 0x00 bytes into symbol "ReadSoundInfo/[nw4r3snd6detail22SoundArchiveFileReaderCFU" @ 0x801C7384
        SyringeCompat::syReplaceFunc(0x801C7384, reinterpret_cast<void*>(ReadSoundInfo), reinterpret_cast<void**>(&_ReadSoundInfo));

#if __LOG_UTILS_ALLOW_LOGGING
        // 0x801BFBDC lands 0x54 bytes into symbol "Alloc/[nw4r3snd6detail9FrameHeapFUlPFPvUlPv_vPv]/snd_Fram" @ 0x801BFB88
        SyringeCompat::syInlineHook(0x801BFBDC, reinterpret_cast<void*>(printFrameHeapAlloc));
        // 0x801BFCBC lands 0x90 bytes into symbol "SaveState/[nw4r3snd6detail9FrameHeapFv]/snd_FrameHeap.o" @ 0x801BFC2C
        SyringeCompat::syInlineHook(0x801BFCBC, reinterpret_cast<void*>(printFrameSaveState));
        // 0x801CAA80 lands 0x58 bytes into symbol "LoadState/[nw4r3snd9SoundHeapFi]/snd_SoundHeap.o" @ 0x801CAA28
        SyringeCompat::syInlineHook(0x801CAA80, reinterpret_cast<void*>(printFrameLoadState));
        // 0x8007A5AC lands 0x288 bytes into symbol "getUseHeapId/[sndHeapSys]/snd_heapsys.o" @ 0x8007A324
        SyringeCompat::syInlineHook(0x8007A5AC, reinterpret_cast<void*>(reportFailedLoad));
#endif
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}