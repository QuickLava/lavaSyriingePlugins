#include <os/OSError.h>
#include <sy_core.h>
#include <fa/fa.h>
#include <modules.h>
#include <string.h>

namespace lavaInjectLoader {

    const char outputTag[] = "[lavaInjectLoader] ";
    static unsigned long* const expansionReturnAddrPtr = 0x800017F0;
    static unsigned long* const expansionCodeAddrPtr = expansionReturnAddrPtr + 0x1;
    static const unsigned long codeBufferLen = 0x2000;
    static const unsigned long returnCodeArr[0x6] =
    {
        0xC0000000, 0x00000002,                                                         // Gecko Pulse (2 Lines):
        0x81FF0000 | ((unsigned long)expansionReturnAddrPtr & 0xFFFF), 0x38800000,      // lwz r15, ReturnAddrLo(r31)       li r4, 0x00
        0x4E800020, 0x00000000                                                          // blr
    };

    enum processStatus
    {
        status_unchecked = -1,
        status_notfound = 0,
        status_continuing,
        status_finishing,
        status_finished,
    };
    static signed long execStatus = status_unchecked;

    void prepareGCT()
    {
        static FAEntryInfo info;
        static char pathBuf[0x80] = {};
        static char codeBuf[codeBufferLen + sizeof(returnCodeArr)] = {};
        static unsigned long folderPathLen;

        if (execStatus == status_finishing)
        {
            execStatus = status_finished;
            OSReport("%sFinished!\n", outputTag);
        }
        if (execStatus == status_finished) return;

        if (execStatus == status_unchecked)
        {
            sprintf(pathBuf, "%spf/injects/", MOD_PATCH_DIR);
            OSReport("%sBeginning Injections from \"%s\"...\n", outputTag, pathBuf);
            folderPathLen = strlen(pathBuf);
            strcpy(pathBuf + folderPathLen, "*.gct");
            execStatus = (FAFsfirst(pathBuf, 0x20, &info) == 0) ? status_continuing : status_finishing;
        }
        if (execStatus == status_continuing && (*expansionCodeAddrPtr == 0x00))
        {
            strcpy(pathBuf + folderPathLen, info.name);
            if (pathBuf[folderPathLen] == 0x0)
            {
                strcpy(pathBuf + folderPathLen, info.shortname);
            }
            OSReport("%sLoading Inject GCT \"%s\"...\n", outputTag, pathBuf);
            FAHandle* streamHandlePtr = FAFopen(pathBuf, (const char*)0x8059C590);
            if (streamHandlePtr != 0x00000000)
            {
                unsigned long fileLength = FAFread(codeBuf, 1, codeBufferLen, streamHandlePtr);

                if ((fileLength - 0x8 + sizeof(returnCodeArr)) < sizeof(codeBuf))
                {
                    *expansionCodeAddrPtr = (unsigned long)codeBuf;
                    unsigned long* returnStart = (unsigned long*)(codeBuf + fileLength - 0x8);
                    for (unsigned long i = 0; i < (sizeof(returnCodeArr) / sizeof(returnCodeArr[0])); i++)
                    {
                        returnStart[i] = returnCodeArr[i];
                    }
                }
                else
                {
                    OSReport("%sGCT overflowed buffer! Aborting!\n", outputTag);
                    execStatus = status_finishing;
                }
            }
            FAFclose(streamHandlePtr);

            if (FAFsnext(&info) != 0)
            {
                execStatus = status_finishing;
            }
        }

        return;
    }
    void installEntryPoint()
    {
        if (execStatus == status_unchecked && *expansionReturnAddrPtr != 0x00)
        {
            SyringeCore::syInlineHook(0x800177B0, (void*)prepareGCT);
        }
    }

    void Init()
    {
        SyringeCore::syInlineHookRel(0x145190, reinterpret_cast<void*>(installEntryPoint), Modules::SORA_MELEE); //0x8084FBA4
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}