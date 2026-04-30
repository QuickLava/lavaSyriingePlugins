#include <os/OSError.h>
#include <sy_compat.h>
#include <OS/OSCache.h>
#include <memory.h>
#include <modules.h>
#include <string.h>

#include "aerialInterrupts.h"
#include "rasterizeVars.h"
#include "customVariables.h"
#include "customEvents.h"

namespace lavaPSAExtensions {

    const char outputTag[] = "[lavaPSAExtensions] ";

    void Init()
    {
        // Note: 0x8070AA14 is SORA_MELEE base address
        aerialInterrupts::registerHooks();
        rasterizeVars::registerHooks();
        customVars::registerHooks();
        customEvents::registerHooks();
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}