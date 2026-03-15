#include <string.h>
#include <sy_compat.h>
#include <revolution/FA.h>
#include "_cmAddonInterface.h"

namespace codeMenu
{
    const char addonFolderPath[] = "Source/CM_Addons/";
    const char locFileFilename[] = "/idx.dat";
    const u32 addonShortNameMaxLen = 0x8;
    const u32 pathBufferLen = sizeof(MOD_PATCH_DIR) + sizeof(addonFolderPath) + addonShortNameMaxLen + sizeof(locFileFilename);

    bool loadCodeMenuAddonLOCsToBuffer(const char* addonShortName, u32* buffer, u32 entryCount)
    {
        bool result = 0;

        char pathBuffer[pathBufferLen];
        if (strlen(addonShortName) <= addonShortNameMaxLen)
        {
            sprintf(pathBuffer, "%s%s%s%s", MOD_PATCH_DIR, addonFolderPath, addonShortName, locFileFilename);
            FAHandle* streamHandlePtr = FAFopen(pathBuffer, (const char*)0x8059C590);
            if (streamHandlePtr != NULL)
            {
                u32 expectedLength = entryCount * 4;
                result = FAFread(buffer, 1, expectedLength, streamHandlePtr) == expectedLength;
                FAFclose(streamHandlePtr);
            }
        }

        return result;
    }
}
