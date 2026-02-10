#include "sy_compat.h"

syApiPtr_t g_API;
namespace SyringeCompat
{
#if SY_COMPAT_TARGET_VER == SY_VER_060
    void syInlineHook(const u32 address, const void* replacement)
    {
        g_API->syInlineHook(address, replacement);
    }
    void syInlineHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        g_API->syInlineHookRel(offset, replacement, moduleId);
    }
    void sySimpleHook(const u32 address, const void* replacement)
    {
        g_API->sySimpleHook(address, replacement);
    }
    void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        g_API->sySimpleHookRel(offset, replacement, moduleId);
    }
    void syReplaceFunc(const u32 address, const void* replacement, void** original)
    {
        g_API->syReplaceFunc(address, replacement, original);
    }
    void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
    {
        g_API->syReplaceFuncRel(offset, replacement, original, moduleId);
    }

    namespace ModuleLoadEvent
    {
        void Subscribe(ModuleLoadCB cb)
        {
            g_API->moduleLoadEventSubscribe(cb);
        }
    }
#elif SY_COMPAT_TARGET_VER == SY_VER_070
    void syInlineHook(const u32 address, const void* replacement)
    {
        g_API->addHookEx(address, replacement, SyringeCore::OPT_ORIG_PRE | SyringeCore::OPT_SAVE_REGS );
    }
    void syInlineHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        g_API->addHookEx(offset, replacement, SyringeCore::OPT_ORIG_PRE | SyringeCore::OPT_SAVE_REGS, moduleId);
    }
    void sySimpleHook(const u32 address, const void* replacement)
    {
        g_API->addHookEx(address, replacement, SyringeCore::OPT_DIRECT);
    }
    void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        g_API->addHookEx(offset, replacement, SyringeCore::OPT_DIRECT, moduleId);
    }
    void syReplaceFunc(const u32 address, const void* replacement, void** original)
    {
        g_API->addHookEx(address, replacement, SyringeCore::OPT_DIRECT)->getTrampoline(original);
    }
    void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
    {
        g_API->addHookEx(offset, replacement, SyringeCore::OPT_DIRECT, moduleId)->getTrampoline(original);
    }

    namespace ModuleLoadEvent
    {
        ModuleLoadCB registeredCallback;
        void moduleLoadEventHandler(::ModuleLoadEvent& loadEvent)
        {
            (*registeredCallback)(loadEvent.getModuleInfo());
        }
        void Subscribe(ModuleLoadCB cb)
        {
            if (registeredCallback == NULL)
            {
                g_API->addEventHandler(Event::ModuleLoad, (SyringeCore::EventHandlerFN)moduleLoadEventHandler);
            }
            else
            {
                OSReport_N("[SY_COMPAT] WARNING: Attempted to register ModuleLoadEvent callback after a callback was already registered! Discarding old callback!\n");
            }
            registeredCallback = cb;
        }
    }
#endif
}