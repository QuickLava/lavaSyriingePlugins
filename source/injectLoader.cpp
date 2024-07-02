#include <os/OSError.h>
#include <sy_core.h>
#include <fa/fa.h>
#include <modules.h>
#include <string.h>

namespace lavaInjectLoader {

    struct NJCTHeader
    {
        static const unsigned long magicVal = 0x4E4A4354; // NJCT

        unsigned long magic;
        unsigned short version;
        unsigned short patchCount;
        unsigned long _reserved;
        unsigned long baseAddr;
    };
    struct NJCTPatchHeader
    {
        unsigned long destinationAddress;
        unsigned long length;
    };

    bool initialized = false;

    void loadInjects()
    {
        initialized = true;

        FAEntryInfo info;
        FAHandle* streamHandlePtr;
        NJCTHeader* currNJCTHeader;
        NJCTPatchHeader* currPatchHeader;

        unsigned long currNJCTBaseAddr;
        unsigned long currNJCTPatchCount;

        unsigned long currPatchWriteAddr;

        char pathBuf[0x80];
        static const unsigned long headerBufLen = (sizeof(NJCTHeader) > sizeof(NJCTPatchHeader)) ? sizeof(NJCTHeader) : sizeof(NJCTPatchHeader);
        char headerBuf[headerBufLen];

        OSReport("Loading Injects from folder!\n");
        sprintf(pathBuf, "%spf/injects/", MOD_PATCH_DIR);
        unsigned long folderPathLen = strlen(pathBuf);
        strcpy(pathBuf + folderPathLen, "*.dat");
        if (FAFsfirst(pathBuf, 0x20, &info) == 0)
        {
            do
            {
                strcpy(pathBuf + folderPathLen, info.name);
                if (pathBuf[folderPathLen] == 0x0)
                {
                    strcpy(pathBuf + folderPathLen, info.shortname);
                }
                OSReport("%s\n", pathBuf);
                streamHandlePtr = FAFopen(pathBuf, (const char*)0x8059C590);
                if (streamHandlePtr != 0x00000000 && 
                    FAFread(headerBuf, 1, sizeof(NJCTHeader), streamHandlePtr) == sizeof(NJCTHeader))
                {
                    currNJCTHeader = (NJCTHeader*)headerBuf;
                    if (currNJCTHeader->magic == NJCTHeader::magicVal)
                    {
                        currNJCTPatchCount = currNJCTHeader->patchCount;
                        currNJCTBaseAddr = currNJCTHeader->baseAddr;

                        while (currNJCTPatchCount > 0x0 &&
                            FAFread(headerBuf, 1, sizeof(NJCTPatchHeader), streamHandlePtr) == sizeof(NJCTPatchHeader))
                        {
                            currPatchHeader = (NJCTPatchHeader*)headerBuf;
                            currPatchWriteAddr = currPatchHeader->destinationAddress;
                            if ((currPatchWriteAddr & 0x80000000) == 0)
                            {
                                currPatchWriteAddr += currNJCTBaseAddr;
                            }
                            FAFread((void*)currPatchWriteAddr, 1, currPatchHeader->length, streamHandlePtr);
                            currNJCTPatchCount -= 0x1;
                        }
                    }
                }
                FAFclose(streamHandlePtr);
            } while (FAFsnext(&info) == 0);
        }
        return;
    }

    void Init()
    {
        SyringeCore::syInlineHookRel(0x145190, reinterpret_cast<void*>(loadInjects), Modules::SORA_MELEE); //0x8084FBA0
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}