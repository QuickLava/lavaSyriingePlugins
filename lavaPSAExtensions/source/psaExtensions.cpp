#include <os/OSError.h>
#include <sy_core.h>
#include <OS/OSCache.h>
#include <fa/fa.h>
#include <gf/gf_scene.h>
#include <gf/gf_heap_manager.h>
#include <memory.h>
#include <modules.h>
#include <string.h>

namespace lavaPSAExtensions {

    void stub() {};

    void Init()
    {
        // Note: 8070AA14 is SORA_MELEE base address
        SyringeCore::syInlineHookRel(0x3591C, reinterpret_cast<void*>(stub), Modules::SORA_MELEE); // 0x80740330
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}