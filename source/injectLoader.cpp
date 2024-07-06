#include <os/OSError.h>
#include <sy_core.h>
#include <fa/fa.h>
#include <modules.h>
#include <string.h>

namespace lavaInjectLoader {

    static unsigned long* const expansionReturnAddrPtr = 0x800017F0;
    static unsigned long* const expansionCodeAddrPtr = expansionReturnAddrPtr + 0x1;
    const unsigned long codeBufferLen = 0x2000;
    const unsigned long returnCodeArr[0x6] =
    {
        0xC0000000, 0x00000002,                                                         // Gecko Pulse (2 Lines):
        0x81FF0000 | ((unsigned long)expansionReturnAddrPtr & 0xFFFF), 0x38800000,      // lwz r15, ReturnAddrLo(r31)       li r4, 0x00
        0x4E800020, 0x00000000                                                          // blr
    };

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
    struct NJCTWordHeader
    {
        unsigned long destinationAddress;
        unsigned long value;
    };

    bool initialized = false;

    void prepareGCT()
    {
        const static signed long status_unchecked = -1;
        const static signed long status_notfound = 0;
        const static signed long status_continuing = 1;
        const static signed long status_finished = 2;
        
        static FAEntryInfo info;
        static char pathBuf[0x80] = {};
        static char codeBuf[codeBufferLen + sizeof(returnCodeArr)] = {};
        static signed long execStatus = status_unchecked;
        static unsigned long folderPathLen;

        if (execStatus == status_unchecked)
        {
            OSReport("[lavaInjectLoader] Beginning Inject GCT from folder...\n");
            sprintf(pathBuf, "%spf/injects/", MOD_PATCH_DIR);
            folderPathLen = strlen(pathBuf);
            strcpy(pathBuf + folderPathLen, "*.gct");
            execStatus = (FAFsfirst(pathBuf, 0x20, &info) == 0) ? status_continuing : status_finished;
        }
        if (execStatus == status_continuing)
        {
            OSReport("[lavaInjectLoader] Loading Inject GCT from folder...\n");
            strcpy(pathBuf + folderPathLen, info.name);
            if (pathBuf[folderPathLen] == 0x0)
            {
                strcpy(pathBuf + folderPathLen, info.shortname);
            }
            OSReport("%s\n", pathBuf);
            FAHandle* streamHandlePtr = FAFopen(pathBuf, (const char*)0x8059C590);
            if (streamHandlePtr != 0x00000000)
            {
                unsigned long fileLength = FAFread(codeBuf, 1, codeBufferLen, streamHandlePtr);

                if ((fileLength - 0x8 + sizeof(returnCodeArr)) < sizeof(codeBuf))
                {
                    unsigned long* returnStart = (unsigned long*)(codeBuf + fileLength - 0x8);


                    for (unsigned long i = 0; i < sizeof(returnCodeArr); i++)
                    {
                        returnStart[i] = returnCodeArr[i];
                    }
                }
                else
                {
                    OSReport("[lavaInjectLoader] GCT overflowed buffer! Aborting!\n");
                    execStatus = status_finished;
                }
            }
            FAFclose(streamHandlePtr);
            *expansionCodeAddrPtr = (unsigned long)codeBuf;

            if (FAFsnext(&info) != 0)
            {
                execStatus = status_finished;
            }
        }
        else if (execStatus == status_finished)
        {
            OSReport("[lavaInjectLoader] Finished!\n");
            *expansionCodeAddrPtr = 0x00ul;
        }

        return;
    }
    void initLoaderAndInstallEntryPoint()
    {
        if (*expansionReturnAddrPtr != 0x00)
        {
            initialized = true;
            SyringeCore::syInlineHook(0x800177B0, (void*)prepareGCT);
        }
    }

    void Init()
    {
        SyringeCore::syInlineHookRel(0x145190, reinterpret_cast<void*>(initLoaderAndInstallEntryPoint), Modules::SORA_MELEE); //0x8084FBA4
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}