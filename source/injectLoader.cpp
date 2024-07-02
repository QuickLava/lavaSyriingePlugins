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

        unsigned long currNJCTPatchCount;

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
                        while (currNJCTPatchCount > 0x0 &&
                            FAFread(headerBuf, 1, sizeof(NJCTPatchHeader), streamHandlePtr) == sizeof(NJCTPatchHeader))
                        {
                            currPatchHeader = (NJCTPatchHeader*)headerBuf;
                            FAFread((void*)currPatchHeader->destinationAddress, 1, currPatchHeader->length, streamHandlePtr);
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
        //SyringeCore::syInlineHook(0x806BFA2C, reinterpret_cast<void*>(loadInjects));
        //SyringeCore::syInlineHookRel(0x108978, reinterpret_cast<void*>(loadInjects), Modules::SORA_MELEE); //0x8081338C
        SyringeCore::syInlineHookRel(0x145190, reinterpret_cast<void*>(loadInjects), Modules::SORA_MELEE); //0x8084FBA0
        //SyringeCore::syInlineHookRel(0x44D8, (void*)loadInjects, Modules::SORA_SCENE);
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}