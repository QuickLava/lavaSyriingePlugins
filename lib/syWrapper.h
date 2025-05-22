#ifndef LAVA_SYRINGE_WRAPPER_V1_H
#define LAVA_SYRINGE_WRAPPER_V1_H

#include <cstdlib>
#include <sy_core.h>

/**
* @brief Event for when modules are loaded.
*/
typedef void (*ModuleLoadCB)(gfModuleInfo*);

#define __ENABLE_LEGACY_HOOK_MODE 0
#if __ENABLE_LEGACY_HOOK_MODE
namespace SyringeCore
{
    void syInlineHook(const u32 address, const void* replacement);
    void syInlineHookRel(const u32 offset, const void* replacement, int moduleId);
    void sySimpleHook(const u32 address, const void* replacement);
    void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId);
    void syReplaceFunc(const u32 address, const void* replacement, void** original);
    void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId);
    namespace ModuleLoadEvent
    {
        void Subscribe(ModuleLoadCB cb);
    }
}
enum SyringeCompatConstants
{
    R3_STACK_BAK_OFF = 0x8,
    R4_STACK_BAK_OFF = R3_STACK_BAK_OFF + 0x4,
};
namespace SyringeCompat
{
#define SYCOMPAT_REGISTER_API
    inline void syInlineHook(const u32 address, const void* replacement)
    {
        SyringeCore::syInlineHook(address, replacement);
    }
    inline void syInlineHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        SyringeCore::syInlineHookRel(offset, replacement, moduleId);
    }
    inline void sySimpleHook(const u32 address, const void* replacement)
    {
        SyringeCore::sySimpleHook(address, replacement);
    }
    inline void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        SyringeCore::sySimpleHookRel(offset, replacement, moduleId);
    }
    inline void syReplaceFunc(const u32 address, const void* replacement, void** original)
    {
        SyringeCore::syReplaceFunc(address, replacement, original);
    }
    inline void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
    {
        SyringeCore::syReplaceFuncRel(offset, replacement, original, moduleId);
    }
    namespace ModuleLoadEvent
    {
        inline void Subscribe(ModuleLoadCB cb)
        {
            SyringeCore::ModuleLoadEvent::Subscribe(cb);
        }
    }
}
#else
enum SyringeCompatConstants
{
    R3_STACK_BAK_OFF = 0xC,
    R4_STACK_BAK_OFF = R3_STACK_BAK_OFF + 0x4,
};
namespace SyringeCompat
{
    extern CoreApi* g_API;
#define SYCOMPAT_REGISTER_API(api_ptr) SyringeCompat::g_API = api_ptr
    /**
     * @brief Injects a hook at the target address. [via syWrapper.h]
     * @note Hooks injected via this function WILL automatically return execution to the original function.
     *
     * @param address address to inject our hook at
     * @param replacement pointer to the function to run
     */
    inline void syInlineHook(const u32 address, const void* replacement)
    {
        g_API->syInlineHook(address, replacement);
    }
    /**
     * @brief Injects an inline hook into a dynamically loaded module on load. [via syWrapper.h]
     * @note Hooks injected via this function WILL automatically return execution to the original function.
     *
     * @param offset offset inside the module's .text section to insert the hook
     * @param replacement pointer to the function to inject
     * @param moduleId ID of the target module
     */
    inline void syInlineHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        g_API->syInlineHookRel(offset, replacement, moduleId);
    }
    /**
     * @brief Injects a simple hook at the target address. [via syWrapper.h]
     * @note Hooks injected through this function WILL NOT automatically branch back to the original after returning.
     *
     * @param address address to inject the hook at
     * @param replacement pointer to function the hook will point to
     */
    inline void sySimpleHook(const u32 address, const void* replacement)
    {
        g_API->sySimpleHook(address, replacement);
    }
    /**
     * @brief Injects a simple hook into a dynamically loaded module on load. [via syWrapper.h]
     * @note Hooks injected through this function WILL NOT automatically branch back to the original after returning.
     *
     * @param offset offset inside the module's .text section to insert the hook
     * @param replacement pointer to function the hook will point to
     * @param moduleId ID of the target module
     */
    inline void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId)
    {
        g_API->sySimpleHookRel(offset, replacement, moduleId);
    }
    /**
     * @brief Replaces the function at the target address with the function pointed to by "replacement". [via syWrapper.h]
     * @note Replacement functions will not automatically call or return to the original function.
     * To call the original function, use the parameter "original"
     *
     * @param address address of the function to replace
     * @param replacement pointer to the replacement function
     * @param original pointer to the original function. Useful for calling the original behavior.
     */
    inline void syReplaceFunc(const u32 address, const void* replacement, void** original)
    {
        g_API->syReplaceFunc(address, replacement, original);
    }
    /**
     * @brief Replaces a function inside of a dynamically loaded module on load. [via syWrapper.h]
     * @note Replacement functions will not automatically call or return to the original function.
     * To call the original function, use the parameter "original"
     *
     * @param offset offset inside the module's .text section of the function to replace
     * @param replacement pointer to the replacement function
     * @param original pointer to the original function. Useful for calling the original behavior.
     * @param moduleId ID of the target module
     */
    inline void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId)
    {
        g_API->syReplaceFuncRel(offset, replacement, original, moduleId);
    }
    
    namespace ModuleLoadEvent
    {
        inline void Subscribe(ModuleLoadCB cb)
        {
            g_API->moduleLoadEventSubscribe(cb);
        }
    }
}
#endif

#endif
