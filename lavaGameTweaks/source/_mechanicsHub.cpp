#include "_mechanicsHub.h"

namespace mechHub
{
    u32 indexBuffer[lid__COUNT] = {};

    char outputTag[] = "[mechHub] ";
    const char addonShortName[] = "MECH_HUB";

    const u32 totalMechanicsCount = amid__COUNT + pmid__COUNT;
    u8 mechanicEnabledMasks[totalMechanicsCount + 1] = {};
    u8* const mechanicsDisabledMask = mechanicEnabledMasks + totalMechanicsCount;
    u8* const activeMechanicEnabledMasks = mechanicEnabledMasks;
    u8* const passiveMechanicEnabledMasks = mechanicEnabledMasks + amid__COUNT;

    u8 const passiveMechanicP1ToggleLineIDs[pmid__COUNT] = 
    { 
        lid_MAGIC_SERIES_TOGGLE_P1, lid_FINAL_SMASH_METER_TOGGLE_P1, lid_FOCUS_ATTACKS_TOGGLE_P1, lid_SQUAT_DODGE_TOGGLE_P1,
        lid_HORI_WAVEDASH_TOGGLE_P1,
    };

    bool getFlagForPlayer(register u8 flagByte, register u32 playerNo)
    {
        register bool result;

        asm
        {
            subfic result, playerNo, 0x20;
            rlwnm result, flagByte, result, 0x1F, 0x1F;
        }

        return result;
    }
    u8 setFlagForPlayer(register u8 flagByte, register u32 playerNo, register bool stateIn)
    {
        register u8 result;

        asm
        {
            li r0, 1;
            rlwnm r0, r0, playerNo, 0x18, 0x1F;
            andc result, flagByte, r0;
            cmplwi stateIn, 0x00;
            b exit;
            or result, flagByte, r0;
        exit:
        }

        return result;
    }
    u8 toggleFlagForPlayer(register u8 flagByte, register u32 playerNo)
    {
        register u8 result;

        asm
        {
            li r0, 1;
            rlwnm r0, r0, playerNo, 0x18, 0x1F;
            xor result, flagByte, r0;
        exit:
        }

        return result;
    }

    void clearMechanicEnabledMasks()
    {
        u8* currMask = mechanicEnabledMasks - 1;
        for (u32 i = 0; i < totalMechanicsCount; i++)
        {
            *(++currMask) = 0;
        }
    }
    void updateMechanicEnabledMasks()
    {
        if (*mechanicsDisabledMask != 0) return;

        u8* currMask = mechanicEnabledMasks - 1;
        for (u32 i = 0; i < amid__COUNT; i++)
        {
            currMask++;
            u8 maskValue = *currMask;
            u8 currMaskBit = 0b1;
            for (u32 u = 0; u < 4; u++)
            {
                u32 activeMechanic = ((codeMenu::cmSelectionLine*)indexBuffer[lid_ACTIVE_MECHANIC_P1 + u])->m_value;
                if (activeMechanic == i)
                {
                    maskValue |= currMaskBit;
                }
                else
                {
                    maskValue &= ~currMaskBit;
                }
                currMaskBit = currMaskBit << 1;
            }
            *currMask = maskValue;
        }
        for (u32 i = 0; i < pmid__COUNT; i++)
        {
            currMask++;
            u32 p1ToggleLineID = passiveMechanicP1ToggleLineIDs[i];
            u8 maskValue = *currMask;
            u8 currMaskBit = 0b1;
            for (u32 u = 0; u < 4; u++)
            {
                u32 mechanicEnabled = ((codeMenu::cmSelectionLine*)indexBuffer[p1ToggleLineID + u])->m_value;
                if (mechanicEnabled != 0)
                {
                    maskValue |= currMaskBit;
                }
                else
                {
                    maskValue &= ~currMaskBit;
                }
                currMaskBit = currMaskBit << 1;
            }
            *currMask = maskValue;
        }
    }

#pragma c99 on
    fighterHooks::callbackBundle callbacks =
    {
        .m_MeleeOnUpdateCB = (fighterHooks::MeleeOnUpdateCB)updateMechanicEnabledMasks,
        .m_MeleeOnGameSetCB = (fighterHooks::MeleeOnGameSetCB)clearMechanicEnabledMasks,
    };
#pragma c99 off

    bool populate()
    {
        bool result = codeMenu::loadCodeMenuAddonLOCsToBuffer(addonShortName, indexBuffer, lid__COUNT);
        if (result)
        {
            OSReport_N("%sSuccessfully Loaded Addon Index File to Buffer 0x%08X!\n", outputTag, indexBuffer);
        }
        else
        {
            *mechanicsDisabledMask = 0xFF;
            OSReport_N("%sFailed to Load Addon Index File to Buffer!\n", outputTag);
        }
        return result;
    }
    void registerHooks()
    {
        fighterHooks::ftCallbackMgr::registerCallbackBundle(&callbacks);
    }

    bool getMechanicEnabled(u32 playerNo, u32 mechanicID, u8* maskArray)
    {
        return (*mechanicsDisabledMask == 0x00) ? getFlagForPlayer(maskArray[mechanicID], playerNo) : false;
    }
    bool getActiveMechanicEnabled(u32 playerNo, activeMechanicIDs mechanicID)
    {
        asm
        {
            lis r5, activeMechanicEnabledMasks@ha;
            lwz r5, activeMechanicEnabledMasks@l(r5);
            bl getMechanicEnabled;
        }
    }
    bool getPassiveMechanicEnabled(u32 playerNo, passiveMechanicIDs mechanicID)
    {
        asm
        {
            lis r5, passiveMechanicEnabledMasks@ha;
            lwz r5, passiveMechanicEnabledMasks@l(r5);
            bl getMechanicEnabled;
        }
    }
}
