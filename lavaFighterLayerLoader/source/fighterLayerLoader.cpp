#include <sy_compat.h>
#include <os/OSError.h>
#include <OS/OSCache.h>
#include <revolution/FA.h>
#include <gf/gf_scene.h>
#include <gf/gf_heap_manager.h>
#include <memory.h>
#include <modules.h>
#include <string.h>
#include <logUtils.h>
#include <ft/fighter.h>
#include <st/st_utility.h>
#include <vector.h>

namespace fighterLayerLoader {

    const char outputTag[] = "[fighterLayerLoader] ";

    void onModuleLoad(gfModuleInfo* moduleIn)
    {
        const char* moduleNameStr = &moduleIn->m_moduleName;
        if (strncmp(moduleNameStr, "ft_", 3) == 0)
        {
            OSReport_N("%sModule Loaded: \"%s\", ModuleAddr:%08X, HeapAddr: %08X\n", outputTag, moduleNameStr, moduleIn, moduleIn->m_heap);

            u32 heapID = 0x00;
            for (u32 i = Heaps::OverlayFighter1; heapID == 0x00 && i <= Heaps::OverlayFighter4; i++)
            {
                void* currHeapAddr = gfHeapManager::getHeap((Heaps::HeapType)i);
                if (currHeapAddr == moduleIn->m_heap)
                {
                    heapID = i;
                }
            }
            if (heapID != 0x00)
            {
                OSReport_N("%sModule Loaded into HeapID %02X!\n", outputTag, heapID);
                Vector<Plugin*>* plugins = g_PLG->getCoreApi()->getRegisteredPlugins();
                const u32 pluginCount = plugins->size();
                for (u32 i = 0; i < pluginCount; i++)
                {
                    Plugin* currPlugin = plugins->get(i);
                    PluginMeta* currPluginMeta = currPlugin->getMetadata();
                    OSReport_N("%sEvaluating Plugin \"%s\"... ", outputTag, currPluginMeta->NAME);
                    if (strncmp(moduleNameStr, currPluginMeta->NAME, strlen(moduleNameStr) - 4) == 0)
                    {
                        OSReport_N("Match!\n");
                        currPluginMeta->FLAGS.heap = heapID;
                        currPlugin->load();
                        currPlugin->execute();
                    }
                    else
                    {
                        OSReport_N("No match!\n");
                    }
                }
            }
        }
    }
    void Init()
    {
        SyringeCompat::ModuleLoadEvent::Subscribe(onModuleLoad);
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}