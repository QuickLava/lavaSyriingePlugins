#include <modules.h>
#include <sy_compat.h>
#include <logUtils.h>
#include <ft/ft_manager.h>
#include <_cmAddonInterface.h>
#include "falconTest.h"

namespace maxiFalconTest {
    const char outputTag[] = "[FalconTest] ";
    const char statusMsg[] = "%s[P%d] %s: %s %s%s!\n";

    const char statusStemStr[] = "statusCaptainFinal";

    u32 getPlayerNo(soModuleAccesser* moduleAccesserIn)
    {
        u32 result = 0xFFFFFFFF;

        if (moduleAccesserIn && moduleAccesserIn->m_stageObject->m_taskCategory == gfTask::Category_Fighter)
        {
            result = g_ftManager->getPlayerNo(((Fighter*)moduleAccesserIn->m_stageObject)->m_entryId);
        }

        return result;
    }

    void(*_initStatusCaptainFinalStart)(u32, soModuleAccesser*);
    void initStatusCaptainFinalStart(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        u32 playerNo = getPlayerNo(moduleAccesserIn);
        if (playerNo < 0xFFFFFFFF)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesserIn->m_stageObject;
            OSReport_N(statusMsg, outputTag, playerNo, fighterPtr->m_taskName, "Init", statusStemStr, "Start");

            moduleAccesserIn->m_enumerationStart->m_soundModule->playSE(snd_se_narration_one, 1, 1, 0);
        }

        // Run original function!
        _initStatusCaptainFinalStart(unk, moduleAccesserIn);
    }
    void(*_initStatusCaptainFinalFuraFura)(u32, soModuleAccesser*);
    void initStatusCaptainFinalFuraFura(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        u32 playerNo = getPlayerNo(moduleAccesserIn);
        if (playerNo < 0xFFFFFFFF)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesserIn->m_stageObject;
            OSReport_N(statusMsg, outputTag, playerNo, fighterPtr->m_taskName, "Init", statusStemStr, "FuraFura");

            moduleAccesserIn->m_enumerationStart->m_soundModule->playSE(snd_se_narration_two, 1, 1, 0);
        }

        // Run original function!
        _initStatusCaptainFinalFuraFura(unk, moduleAccesserIn);
    }
    void(*_initStatusCaptainFinalClash)(u32, soModuleAccesser*);
    void initStatusCaptainFinalClash(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        u32 playerNo = getPlayerNo(moduleAccesserIn);
        if (playerNo < 0xFFFFFFFF)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesserIn->m_stageObject;
            OSReport_N(statusMsg, outputTag, playerNo, fighterPtr->m_taskName, "Init", statusStemStr, "Clash");

            moduleAccesserIn->m_enumerationStart->m_soundModule->playSE(snd_se_narration_three, 1, 1, 0);
        }

        // Run original function!
        _initStatusCaptainFinalClash(unk, moduleAccesserIn);
    }
    void(*_initStatusCaptainFinalEnd)(u32, soModuleAccesser*);
    void initStatusCaptainFinalEnd(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        u32 playerNo = getPlayerNo(moduleAccesserIn);
        if (playerNo < 0xFFFFFFFF)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesserIn->m_stageObject;
            OSReport_N(statusMsg, outputTag, playerNo, fighterPtr->m_taskName, "Init", statusStemStr, "End");

            moduleAccesserIn->m_enumerationStart->m_soundModule->playSE(snd_se_narration_four, 1, 1, 0);
        }

        // Run original function!
        _initStatusCaptainFinalEnd(unk, moduleAccesserIn);
    }

    void(*_execStatusCaptainFinalStart)(u32, soModuleAccesser*);
    void execStatusCaptainFinalStart(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        u32 playerNo = getPlayerNo(moduleAccesserIn);
        if (playerNo < 0xFFFFFFFF)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesserIn->m_stageObject;
            OSReport_N(statusMsg, outputTag, playerNo, fighterPtr->m_taskName, "Exec", statusStemStr, "Start");
        }

        // Run original function!
        _execStatusCaptainFinalStart(unk, moduleAccesserIn);
    }
    void(*_execStatusCaptainFinalFuraFura)(u32, soModuleAccesser*);
    void execStatusCaptainFinalFuraFura(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        u32 playerNo = getPlayerNo(moduleAccesserIn);
        if (playerNo < 0xFFFFFFFF)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesserIn->m_stageObject;
            OSReport_N(statusMsg, outputTag, playerNo, fighterPtr->m_taskName, "Exec", statusStemStr, "FuraFura");
        }

        // Run original function!
        _execStatusCaptainFinalFuraFura(unk, moduleAccesserIn);
    }
    void(*_execStatusCaptainFinalClash)(u32, soModuleAccesser*);
    void execStatusCaptainFinalClash(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        u32 playerNo = getPlayerNo(moduleAccesserIn);
        if (playerNo < 0xFFFFFFFF)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesserIn->m_stageObject;
            OSReport_N(statusMsg, outputTag, playerNo, fighterPtr->m_taskName, "Exec", statusStemStr, "Clash");
        }

        // Run original function!
        _execStatusCaptainFinalClash(unk, moduleAccesserIn);
    }
    void(*_execStatusCaptainFinalEnd)(u32, soModuleAccesser*);
    void execStatusCaptainFinalEnd(u32 unk, soModuleAccesser* moduleAccesserIn)
    {
        u32 playerNo = getPlayerNo(moduleAccesserIn);
        if (playerNo < 0xFFFFFFFF)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesserIn->m_stageObject;
            OSReport_N(statusMsg, outputTag, playerNo, fighterPtr->m_taskName, "Exec", statusStemStr, "End");
        }

        // Run original function!
        _execStatusCaptainFinalEnd(unk, moduleAccesserIn);
    }

    void Init()
    {
        SyringeCompat::syReplaceFuncRel(0x1902EC,
            reinterpret_cast<void*>(initStatusCaptainFinalStart),
            reinterpret_cast<void**>(&_initStatusCaptainFinalStart),
            Modules::SORA_MELEE);
        SyringeCompat::syReplaceFuncRel(0x1903AC,
            reinterpret_cast<void*>(initStatusCaptainFinalFuraFura),
            reinterpret_cast<void**>(&_initStatusCaptainFinalFuraFura),
            Modules::SORA_MELEE);
        SyringeCompat::syReplaceFuncRel(0x190924,
            reinterpret_cast<void*>(initStatusCaptainFinalClash),
            reinterpret_cast<void**>(&_initStatusCaptainFinalClash),
            Modules::SORA_MELEE);
        SyringeCompat::syReplaceFuncRel(0x190C58,
            reinterpret_cast<void*>(initStatusCaptainFinalEnd),
            reinterpret_cast<void**>(&_initStatusCaptainFinalEnd),
            Modules::SORA_MELEE);

        SyringeCompat::syReplaceFuncRel(0x1903A8,
            reinterpret_cast<void*>(execStatusCaptainFinalStart),
            reinterpret_cast<void**>(&_execStatusCaptainFinalStart),
            Modules::SORA_MELEE);
        SyringeCompat::syReplaceFuncRel(0x1904B8,
            reinterpret_cast<void*>(execStatusCaptainFinalFuraFura),
            reinterpret_cast<void**>(&_execStatusCaptainFinalFuraFura),
            Modules::SORA_MELEE);
        SyringeCompat::syReplaceFuncRel(0x1909C8,
            reinterpret_cast<void*>(execStatusCaptainFinalClash),
            reinterpret_cast<void**>(&_execStatusCaptainFinalClash),
            Modules::SORA_MELEE);
        SyringeCompat::syReplaceFuncRel(0x190CB0,
            reinterpret_cast<void*>(execStatusCaptainFinalEnd),
            reinterpret_cast<void**>(&_execStatusCaptainFinalEnd),
            Modules::SORA_MELEE);
    }

    void Destroy(){}
}
