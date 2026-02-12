#ifndef LAVA_SYRINGE_COMPAT_V1_H
#define LAVA_SYRINGE_COMPAT_V1_H

#include <gf/gf_file_io_request.h>
#include <gf/gf_memory_pool.h>
#include <sr/sr_common.h>
#include <gf/gf_module.h>
#include <cstdlib>
#include "logUtils.h"

#define SY_VER_050 0
#define SY_VER_060 1
#define SY_VER_070 2
#define SY_COMPAT_TARGET_VER SY_VER_070

#if SY_COMPAT_TARGET_VER == SY_VER_050

enum stackElement
{
    se__PrevStackFrame = 0x00,
    se__ReservedLinkRegBackup,
    se_Reg03, se_Reg04, se_Reg05, se_Reg06,
    se_Reg07, se_Reg08, se_Reg09, se_Reg10,
    se_Reg11, se_Reg12, se_Reg13, se_Reg14,
    se_Reg15, se_Reg16, se_Reg17, se_Reg18,
    se_Reg19, se_Reg20, se_Reg21, se_Reg22,
    se_Reg23, se_Reg24, se_Reg25, se_Reg26,
    se_Reg27, se_Reg28, se_Reg29, se_Reg30,
    se_Reg31,
    se__ElementCount
};

#elif SY_COMPAT_TARGET_VER == SY_VER_060
#include <sy_core_060/include/plugin.h>
typedef CoreApi* syApiPtr_t;
extern syApiPtr_t g_API;

enum stackElement
{
    se__PrevStackFrame = 0x00,
    se__ReservedLinkRegBackup,
    se_Reg00,
    se_Reg03, se_Reg04, se_Reg05, se_Reg06,
    se_Reg07, se_Reg08, se_Reg09, se_Reg10,
    se_Reg11, se_Reg12, se_Reg13, se_Reg14,
    se_Reg15, se_Reg16, se_Reg17, se_Reg18,
    se_Reg19, se_Reg20, se_Reg21, se_Reg22,
    se_Reg23, se_Reg24, se_Reg25, se_Reg26,
    se_Reg27, se_Reg28, se_Reg29, se_Reg30,
    se_Reg31,
    se__ElementCount
};

// Creates extern "C" block.
#define SY_COMPAT_EXTERN_C_BLOCK  extern "C" {                       \
    typedef void (*PFN_voidfunc)();                                  \
    __attribute__((section(".ctors"))) extern PFN_voidfunc _ctors[]; \
    __attribute__((section(".ctors"))) extern PFN_voidfunc _dtors[]; \
    const PluginMeta* _prolog(syApiPtr_t api); void _epilog(); void _unresolved(); }
// Creates PluginMeta META object.
#define SY_COMPAT_PLG_META(pluginName, author, version, syVersion) const PluginMeta META = { pluginName, author, Version(version), Version(syVersion) };   
// Run Global Constructors and (on Syriinge API 0.7.0) runs provided entry function.
#define SY_COMPAT_PROLOG_FN(entryFn) const PluginMeta* _prolog(syApiPtr_t api) \
{ for (PFN_voidfunc* ctor = _ctors; *ctor; ctor++) { (*ctor)(); } g_API = api; entryFn(); return &META; }
// On Syriinge API 0.7.0, runs the passed in function. On 0.6.0, does nothing.
#define SY_COMPAT_MAIN_FN(entryFn) void main(void) { }

#elif SY_COMPAT_TARGET_VER == SY_VER_070
#include <sy_core_070/include/hook.hpp>
#include <sy_core_070/include/plugin.hpp>
#include <sy_core_070/include/sy_core.hpp>
typedef Plugin* syApiPtr_t;
extern syApiPtr_t g_API;

enum stackElement
{
    se__PrevStackFrame = 0x00,
    se__ReservedLinkRegBackup,
    se_Reg00,
    se_Reg03, se_Reg04, se_Reg05, se_Reg06,
    se_Reg07, se_Reg08, se_Reg09, se_Reg10,
    se_Reg11, se_Reg12, se_Reg13, se_Reg14,
    se_Reg15, se_Reg16, se_Reg17, se_Reg18,
    se_Reg19, se_Reg20, se_Reg21, se_Reg22,
    se_Reg23, se_Reg24, se_Reg25, se_Reg26,
    se_Reg27, se_Reg28, se_Reg29, se_Reg30,
    se_Reg31,
    se__ElementCount
};

// Creates extern "C" block.
#define SY_COMPAT_EXTERN_C_BLOCK extern "C" {                                                   \
    typedef void (*PFN_voidfunc)();                                                             \
    __attribute__((section(".ctors"))) extern PFN_voidfunc _ctors[];                            \
    __attribute__((section(".ctors"))) extern PFN_voidfunc _dtors[];                            \
    const PluginMeta* _prolog(); void _epilog(); void _unresolved(); void main(syApiPtr_t api); \
}
// Creates PluginMeta META object.
#define SY_COMPAT_PLG_META(pluginName, author, version, syVersion) const PluginMeta META = {  \
    pluginName, author, Version(version), Version(syVersion),                                 \
    main, .FLAGS = { .timing = TIMING_BOOT, .loading = LOAD_PERSIST, .heap = Heaps::Syringe } \
};
// Run Global Constructors and (on Syriinge API 0.7.0) runs provided entry function.
#define SY_COMPAT_PROLOG_FN(entryFn) const PluginMeta* _prolog() \
{ for (PFN_voidfunc* ctor = _ctors; *ctor; ctor++) { (*ctor)(); } return &META; }
// On Syriinge API 0.7.0, runs the passed in function. On 0.6.0, does nothing.
#define SY_COMPAT_MAIN_FN(entryFn) void main(syApiPtr_t api) { g_API = api; entryFn(); }
#endif

// Runs Global Destructors!
#define SY_COMPAT_EPILOG_FN void _epilog() { for (PFN_voidfunc* dtor = _dtors; *dtor; dtor++) { (*dtor)(); } }
// _unresolved function definition.
#define SY_COMPAT_UNRESOLVED_FN void _unresolved(void) { }

// Cross-version compatible API wrapper.
namespace SyringeCompat
{
#define STACK_ELEMENT_OFFSET(SyCompat_stackElement) SyCompat_stackElement * 4

    /**
     * @brief Injects a hook at the target address. [via sy_compat.h]
     * @note Hooks injected via this function WILL automatically return execution to the original function.
     *
     * @param address address to inject our hook at
     * @param replacement pointer to the function to run
     */
    void syInlineHook(const u32 address, const void* replacement);
    /**
     * @brief Injects an inline hook into a dynamically loaded module on load. [via sy_compat.h]
     * @note Hooks injected via this function WILL automatically return execution to the original function.
     *
     * @param offset offset inside the module's .text section to insert the hook
     * @param replacement pointer to the function to inject
     * @param moduleId ID of the target module
     */
    void syInlineHookRel(const u32 offset, const void* replacement, int moduleId);
    /**
     * @brief Injects a simple hook at the target address. [via sy_compat.h]
     * @note Hooks injected through this function WILL NOT automatically branch back to the original after returning.
     *
     * @param address address to inject the hook at
     * @param replacement pointer to function the hook will point to
     */
    void sySimpleHook(const u32 address, const void* replacement);
    /**
     * @brief Injects a simple hook into a dynamically loaded module on load. [via sy_compat.h]
     * @note Hooks injected through this function WILL NOT automatically branch back to the original after returning.
     *
     * @param offset offset inside the module's .text section to insert the hook
     * @param replacement pointer to function the hook will point to
     * @param moduleId ID of the target module
     */
    void sySimpleHookRel(const u32 offset, const void* replacement, int moduleId);
    /**
     * @brief Replaces the function at the target address with the function pointed to by "replacement". [via sy_compat.h]
     * @note Replacement functions will not automatically call or return to the original function.
     * To call the original function, use the parameter "original"
     *
     * @param address address of the function to replace
     * @param replacement pointer to the replacement function
     * @param original pointer to the original function. Useful for calling the original behavior.
     */
    void syReplaceFunc(const u32 address, const void* replacement, void** original);
    /**
     * @brief Replaces a function inside of a dynamically loaded module on load. [via sy_compat.h]
     * @note Replacement functions will not automatically call or return to the original function.
     * To call the original function, use the parameter "original"
     *
     * @param offset offset inside the module's .text section of the function to replace
     * @param replacement pointer to the replacement function
     * @param original pointer to the original function. Useful for calling the original behavior.
     * @param moduleId ID of the target module
     */
    void syReplaceFuncRel(const u32 offset, const void* replacement, void** original, int moduleId);

    /**
    * @brief Event for when modules are loaded.
    */
    typedef void (*ModuleLoadCB)(gfModuleInfo*);
    namespace ModuleLoadEvent
    {
        void Subscribe(ModuleLoadCB cb);
    }
}

#endif
