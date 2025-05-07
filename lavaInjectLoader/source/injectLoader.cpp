#include <os/OSError.h>
#include <sy_core.h>
#include <OS/OSCache.h>
#include <revolution/FA.h>
#include <gf/gf_scene.h>
#include <gf/gf_heap_manager.h>
#include <memory.h>
#include <modules.h>
#include <string.h>

namespace lavaInjectLoader {

    const char outputTag[] = "[lavaInjectLoader] ";
    const char fighterInjectPath[] = "pf/injects/fighter/";
    static unsigned long* const expansionReturnAddrPtr = 0x800017F0;
    static unsigned long* const expansionCodeAddrPtr = expansionReturnAddrPtr + 0x1;
    static unsigned long* const expansionSourcePathPtr = expansionCodeAddrPtr + 0x1;

    static const unsigned long returnCodeArr[0x6] = {
        0xC0000000, 0x00000002,                                                         // Gecko Pulse (2 Lines):
        0x81FF0000 | ((unsigned long)expansionReturnAddrPtr & 0xFFFF), 0x38800000,      // lwz r15, ReturnAddrLo(r31)       li r4, 0x00
        0x4E800020, 0x00000000                                                          // blr
    };
    static const unsigned long maxGCTLen = 0x2000;
    static const unsigned long codeBufferLen = maxGCTLen + sizeof(returnCodeArr) - 0x8;
    static char* codeBuf = NULL;

    enum processStatus
    {
        status_idle = 0,
        status_working,
        status_finishing,
    };
    static signed long execStatus = status_idle;

    void freeCodeBuffer()
    {
        // Optional logging logic.
        /*gfSceneManager* sceneManager = gfSceneManager::getInstance();
        if (sceneManager != NULL && sceneManager->m_currentScene != NULL)
        {
            OSReport("%s[FREE] Current Scene: %s", outputTag, sceneManager->m_currentScene->m_sceneName);
        }*/

        // If a buffer is currently allocated...
        if (codeBuf != NULL)
        {
            // ... release the allocated region...
            free(codeBuf);
            // ... and null the buffer pointer.
            codeBuf = NULL;
        }
    }
    bool allocCodeBuffer(HeapType destinationHeap, unsigned long allocSize)
    {
        // Optional logging logic.
        /*gfSceneManager* sceneManager = gfSceneManager::getInstance();
        if (sceneManager != NULL && sceneManager->m_currentScene != NULL)
        {
            OSReport("%s[FREE] Current Scene: %s", outputTag, sceneManager->m_currentScene->m_sceneName);
        }*/

        // If a buffer is currently allocated...
        if (codeBuf != NULL)
        {
            // ... deallocate it before continuing.
            freeCodeBuffer();
        }
        // Then, allocate a new buffer with the given specifications and store its address in codeBufferPtr...
        codeBuf = (char*)gfHeapManager::alloc(destinationHeap, allocSize);
        // ...and return whether or not we succeded.
        return  codeBuf != NULL;
    }

    void prepareGCT()
    {
        static FAEntryInfo info;
        static char pathBuf[0x80] = {};
        static unsigned long folderPathLen;

        // Don't even try to process anything if we don't have a defined return address or code buffer, just return immediately.
        if (*expansionReturnAddrPtr == 0x00 || codeBuf == NULL) return;

        // If the previous round of work we did finished up our batch...
        if (execStatus == status_finishing && (*expansionCodeAddrPtr == 0x00))
        {
            // ... free the code buffer...
            freeCodeBuffer();
            // ... set us back to idle...
            execStatus = status_idle;
            // ... zero out the source path to mark that we're open for a new batch...
            *expansionSourcePathPtr = 0x00000000;
            // ... and log that we've finished!
            // Note: this is ahead of the below so we don't do this stuff on the same call where we queue the last .gct lol.
            OSReport("%sFinished!\n", outputTag);
        }
        // If we're idle, and some work's been queued up...
        if (execStatus == status_idle && (*expansionSourcePathPtr != 0x00))
        {
            // ... then get the full path to the source directory...
            sprintf(pathBuf, "%s%s", MOD_PATCH_DIR, *expansionSourcePathPtr);
            // ... and report that we're attempting to apply it.
            OSReport("%sBeginning Injections from \"%s\"...\n", outputTag, pathBuf);
            // Record the length of just the path so we know where to append filenames to.
            folderPathLen = strlen(pathBuf);
            // Set up the pattern string for our file search...
            strcpy(pathBuf + folderPathLen, "*.gct");
            // ... and attempt to get the first file which matches it!
            // If we find something, we'll switch to operating mode, if we don't we'll just skip to finishing.
            execStatus = (FAFsfirst(pathBuf, 0x20, &info) == 0) ? status_working : status_finishing;
        }
        // If we're working, and no .gct is currently waiting to be processed...
        if (execStatus == status_working && (*expansionCodeAddrPtr == 0x00))
        {
            // ... set up a new one!
            // Set up the full path to the new target file (accounting for the case where we need short name instead).
            strcpy(pathBuf + folderPathLen, info.name);
            if (pathBuf[folderPathLen] == 0x0)
            {
                strcpy(pathBuf + folderPathLen, info.shortname);
            }
            // Report the path of the GCT we're loading...
            OSReport("%sLoading Inject GCT \"%s\"... to 0x%X8\n", outputTag, pathBuf, codeBuf);
            // ... then attempt to open a stream to it!
            FAHandle* streamHandlePtr = FAFopen(pathBuf, (const char*)0x8059C590);
            // If that was successfull...
            if (streamHandlePtr != 0x00000000)
            {
                // ... then read in as much of the file as we can fit in the buffer, recording the actual read-in length.
                unsigned long fileLength = FAFread(codeBuf, 1, codeBufferLen, streamHandlePtr);
                // If what we read in didn't reach or exceed the allowed amount...
                if (fileLength < maxGCTLen)
                {
                    // ... then write the buffer address into place, so that the bootstrapper can apply it!
                    *expansionCodeAddrPtr = (unsigned long)codeBuf;
                    // Then, write the return code into place!
                    unsigned long* returnStart = (unsigned long*)(codeBuf + fileLength - 0x8);
                    for (unsigned long i = 0; i < (sizeof(returnCodeArr) / sizeof(returnCodeArr[0])); i++)
                    {
                        returnStart[i] = returnCodeArr[i];
                    }
                }
                // If it did exceed the allowed space though...
                else
                {
                    // ... report it, and abort loading the batch.
                    OSReport("%sGCT overflowed buffer! Aborting!\n", outputTag);
                    execStatus = status_finishing;
                }
                ICInvalidateRange((void*)codeBuf, fileLength + 0x10);
                DCFlushRange((void*)codeBuf, fileLength + 0x10);

                // Regardless, ensure we close the stream handle before we exit!
                FAFclose(streamHandlePtr);
            }
            // Lastly, attempt to queue up the next .gct to load! If we fail though...
            if (FAFsnext(&info) != 0)
            {
                // ... then we're finished, so mark that we're finishing up the batch!
                execStatus = status_finishing;
            }
        }
        
        return;
    }
    void doFighterInjectLoads()
    {
        if (*expansionReturnAddrPtr != 0x00 && allocCodeBuffer(Heaps::OverlayFighter1, codeBufferLen))
        {
            *expansionSourcePathPtr = (unsigned long)fighterInjectPath;
        }
    }

    void Init(CoreApi* api)
    {
        SyringeCore::syInlineHook(0x800177A8, reinterpret_cast<void*>(prepareGCT));
        SyringeCore::syInlineHookRel(0x145190, reinterpret_cast<void*>(doFighterInjectLoads), Modules::SORA_MELEE); //0x8084FBA4
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}