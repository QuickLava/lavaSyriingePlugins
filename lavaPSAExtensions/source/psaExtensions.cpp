#include <os/OSError.h>
#include <sy_core.h>
#include <OS/OSCache.h>
#include <memory.h>
#include <modules.h>
#include <string.h>

#include "aerialInterrupts.h"

namespace lavaPSAExtensions {

    const char outputTag[] = "[lavaPSAExtensions] ";

    void Init()
    {
        // Note: 0x8070AA14 is SORA_MELEE base address
        aerialInterrupts::registerHooks();
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}