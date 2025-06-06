#include <gf/gf_file_io_request.h>
#include <gf/gf_memory_pool.h>
#include <syWrapper.h>

#include "gameTweaks.h"

namespace Syringe
{
    const PluginMeta META = {
        "lavaGameTweaks",
        "QuickLava",
        Version("0.1.5"),
        Version(SYRINGE_VERSION)};

    extern "C"
    {
        typedef void (*PFN_voidfunc)();
        __attribute__((section(".ctors"))) extern PFN_voidfunc _ctors[];
        __attribute__((section(".ctors"))) extern PFN_voidfunc _dtors[];

        const PluginMeta *_prolog(CoreApi* api);
        void _epilog();
        void _unresolved();
    }

    const PluginMeta *_prolog(CoreApi* api)
    {
        // Run global constructors
        PFN_voidfunc *ctor;
        for (ctor = _ctors; *ctor; ctor++)
        {
            (*ctor)();
        }

        SYCOMPAT_REGISTER_API(api);
        lavaGameTweaks::Init();

        return &META;
    }

    void _epilog()
    {
        // run the global destructors
        PFN_voidfunc *dtor;
        for (dtor = _dtors; *dtor; dtor++)
        {
            (*dtor)();
        }
    }

    void _unresolved(void)
    {
    }

}