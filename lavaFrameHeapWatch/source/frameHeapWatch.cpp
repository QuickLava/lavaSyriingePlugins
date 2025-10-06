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
        register u32* disposeFunc;
        asm
        {
            mr destination, r3;
            mr frameHeap, r27;
            subi soundHeap, frameHeap, 0x1C;
            mr size, r28;
            mr disposeFunc, r29;
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

    void Init()
    {
        // 0x801BFBDC lands 0x54 bytes into symbol "Alloc/[nw4r3snd6detail9FrameHeapFUlPFPvUlPv_vPv]/snd_Fram" @ 0x801BFB88
        SyringeCompat::syInlineHook(0x801BFBDC, reinterpret_cast<void*>(printFrameHeapAlloc));
        // 0x801BFCBC lands 0x90 bytes into symbol "SaveState/[nw4r3snd6detail9FrameHeapFv]/snd_FrameHeap.o" @ 0x801BFC2C
        SyringeCompat::syInlineHook(0x801BFCBC, reinterpret_cast<void*>(printFrameSaveState));
        // 0x801CAA80 lands 0x58 bytes into symbol "LoadState/[nw4r3snd9SoundHeapFi]/snd_SoundHeap.o" @ 0x801CAA28
        SyringeCompat::syInlineHook(0x801CAA80, reinterpret_cast<void*>(printFrameLoadState));
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}