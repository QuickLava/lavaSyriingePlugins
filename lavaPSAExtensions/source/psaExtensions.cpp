#include <os/OSError.h>
#include <syWrapper.h>
#include <OS/OSCache.h>
#include <memory.h>
#include <modules.h>
#include <string.h>

#include "aerialInterrupts.h"
#include "rasterizeVars.h"
#include "customVariables.h"

namespace lavaPSAExtensions {

    const char outputTag[] = "[lavaPSAExtensions] ";

    void Init()
    {
        // Note: 0x8070AA14 is SORA_MELEE base address
        aerialInterrupts::registerHooks();
        rasterizeVars::registerHooks();
        customVars::registerHooks();
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}