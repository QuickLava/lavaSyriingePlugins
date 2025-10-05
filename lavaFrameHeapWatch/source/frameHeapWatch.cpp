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

    extern u32 GetFreeSize(u32* frameHeap);
    extern u32 GetCurrentLevel(u32* frameHeap);
    extern u32  MEMGetAllocatableSizeForFrmHeapEx(u32 heap, u32 padding);
    void printFrameHookAlloc()
    {
        register u32* destination;
        register u32* frameHeap;
        register u32 size;
        register u32* disposeFunc;
        asm
        {
            mr destination, r3;
            mr frameHeap, r27;
            mr size, r28;
            mr disposeFunc, r29;
        }
        u32 freeSpace = GetFreeSize(frameHeap);
        u32 current = GetCurrentLevel(frameHeap);

        u32* heapSys = *(((u32**)g_sndSystem) + 0xB4);
        u32* soundHeapArr = heapSys + 0x96;
        u32* soundHeap = frameHeap - 0x7;
        u32 soundHeapDist = (soundHeap - soundHeapArr) * 4;
        u32 soundHeapID = soundHeapDist / 0x2C;
        OSReport_N("%sAlloc: FrameHeap[%02X] @ %08X, Size: %08X (FreeSize: %08X, CurrLevel: %02X)\n", outputTag, soundHeapID, destination, size, freeSpace, current);
        //OSReport_N("%s- SoundHeapAddr: %08X, SoundHeapArr: %08X, SoundHeapDist: %08X\n", outputTag, soundHeap, soundHeapArr, soundHeapDist);
    }

    void Init()
    {
        // 0x801BFBDC lands 0x54 bytes into symbol "Alloc/[nw4r3snd6detail9FrameHeapFUlPFPvUlPv_vPv]/snd_Fram" @ 0x801BFB88
        SyringeCompat::syInlineHook(0x801BFBDC, reinterpret_cast<void*>(printFrameHookAlloc));
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}