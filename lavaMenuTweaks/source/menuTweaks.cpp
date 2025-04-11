#include <sy_core.h>
#include <modules.h>
#include <logUtils.h>
#include <gf/gf_pad_system.h>
#include <gm/gm_sel_char_data.h>
#include <mu/selchar/mu_selchar_player_area.h>

namespace lavaMenuTweaks {

    const char outputTag[] = "[lavaMenuTweaks] ";

    u8** altRequestAreaAddrLoc = (u8**)0x8084CE38;

    const u32 maxChars = 0x80;
    const u32 altFrameFlagBlockCount = maxChars / 0x20;
    enum requestedAlt
    {
        altNone = 0,
        altZ = 1,
        altR = 2,
        altNum = 3,
    };

    struct altFrameFlagBlock
    {
        u32 frameFlags[3];
    };
    altFrameFlagBlock altFrameFlagBuffer[altFrameFlagBlockCount];

    struct PAT0Header
    {
        // "PAT0"
        u32 magicNum;
        u32 size;
        u32 version;
        s32 brresOffset;

        s32 dataOffset;
        s32 texTableOffset; //String list 1
        s32 pltTableOffset; //String list 2
        s32 texPtrTableOffset;

        s32 pltPtrTableOffset;
        s32 stringOffset;
        s32 origPathOffset;

        u16 numFrames;
        u16 numEntries; //Same as group entries, directly below
        u16 numTexPtr;
        u16 numPltPtr;

        u32 loop;
    };
    
    struct PAT0MaterialEntry
    {
        enum flags
        {
            f_ENABLED = 0x1,
            f_UNK2 = 0x2,
            f_HAS_TEX = 0x4,
            f_HAS_PLT = 0x8,
        };

        s32 stringOffset;
        u32 flags;
        u32 tableOffset;
    };
    struct PAT0KeyframeTableHeader
    {
        u16 textureCount;
        u16 pad;
        float frameScale; // == 1 / last entry's key
    };
    struct PAT0Keyframe
    {
        float frame;
        u16 textureIndex;
        u16 paletteIndex;
    };

    u32 getStockChKind(MuSelchkind selCharID)
    {
        return muMenu::exchangeGmCharacterKind2Something(muMenu::exchangeMuSelchkind2MuStockchkind(selCharID));
    }
    u32 getPAT0FrameFromSelcharID(MuSelchkind selCharID)
    {
        return (getStockChKind(selCharID) * 10) + 1;
    }
    bool frameForAltExists(MuSelchkind selCharID, u32 altKind)
    {
        bool result = 0;

        u32 convertedCharID = getStockChKind(selCharID);
        u32 targetBlockIndex = convertedCharID >> 0x5;
        if (targetBlockIndex < altFrameFlagBlockCount && altKind > 0)
        {
            u32 targetBit = 1 << (convertedCharID & 0x1F);
            result = altFrameFlagBuffer[targetBlockIndex].frameFlags[altKind - 1] & targetBit;
        }

        return result;
    }

    bool buildNumAltEnabledBuffer(muSelCharTask* selCharTask)
    {
        bool result = 0;

        register nw4r::g3d::AnmObjTexPatRes* charNumPat0 = selCharTask->m_selCharPlayerAreas[0]->m_muCharName->m_modelAnim->m_anmObjTexPatRes;
        OSReport_N("%sPat0 Obj and File Address: %08X, %08X\n", outputTag, (u32)charNumPat0, (u32)charNumPat0->m_anmTexPatFile.ptr());

        PAT0Header* pat0HeaderPtr = (PAT0Header*)charNumPat0->m_anmTexPatFile.ptr();
        char* pat0BodyPtr = (char*)pat0HeaderPtr + pat0HeaderPtr->dataOffset;

        PAT0MaterialEntry* matStructPtr = (PAT0MaterialEntry*)(pat0BodyPtr + *(s32*)(pat0BodyPtr + 0x24));
        PAT0KeyframeTableHeader* matTablePtr = (PAT0KeyframeTableHeader*)((char*)matStructPtr + matStructPtr->tableOffset);
        u32 frameCount = matTablePtr->textureCount;
        PAT0Keyframe* frameArr = ((PAT0Keyframe*)matTablePtr) + 1;

        u32 zAltFlags;
        u32 rAltFlags;
        u32 numAltFlags;
        u32 currBlockIndex = 0x00;
        u32 currFrameIndex = 0x00;
        OSReport_N("%sALT_FRAME_FLAG_BUF @ %08X:\n", outputTag, altFrameFlagBuffer);
        for (u32 currBlockIndex = 0x00, targetBlockIndex = 0x00; currBlockIndex < altFrameFlagBlockCount; currBlockIndex++)
        {
            zAltFlags = 0x00;
            rAltFlags = 0x00;
            numAltFlags = 0x00;
            while (currFrameIndex < frameCount)
            {
                u32 frameInt = (u32)(frameArr[currFrameIndex].frame);
                u32 targetCharacter = frameInt / 10;
                u32 altIndex = (frameInt - (targetCharacter * 10));
                targetBlockIndex = targetCharacter >> 0x05;
                if (currBlockIndex != targetBlockIndex) break;

                u32 targetCharFlag = 1 << (targetCharacter & 0x1F);
                switch (altIndex)
                {
                    case 2: { zAltFlags |= targetCharFlag; break; }
                    case 3: { rAltFlags |= targetCharFlag; break; }
                    case 4: { numAltFlags |= targetCharFlag; break; }
                }
                currFrameIndex++;
            }
            OSReport_N("%s- Z: %08X, R: %08X, Num: %08X\n", outputTag, zAltFlags, rAltFlags, numAltFlags);
            altFrameFlagBlock* currBlock = altFrameFlagBuffer + currBlockIndex;
            currBlock->frameFlags[0] = zAltFlags;
            currBlock->frameFlags[1] = rAltFlags;
            currBlock->frameFlags[2] = numAltFlags;
        }

        return result;
    }
    void setCharNameFrameFromCurrAlt(muSelCharPlayerArea* playerArea)
    {
        MuSelchkind charKind = playerArea->m_charKind;
        if (charKind != Selch_SelectNone)
        {
            u8* altRequestArea = *altRequestAreaAddrLoc;
            u32 portNo = playerArea->m_areaIdx;
            u32 requested = altRequestArea[portNo];

            u32 charNameFrame = getPAT0FrameFromSelcharID(Selch_Random);
            if (requested == 0x00 || frameForAltExists(charKind, requested))
            {
                charNameFrame = getPAT0FrameFromSelcharID(charKind);
            }
            playerArea->m_muCharName->setFrameTex(charNameFrame + requested);
            OSReport_N("%sPlayerArea Port No %d: CharName Frame: %d\n", outputTag, portNo, charNameFrame + requested);
        }
    }
    
    void onSetCharKind()
    {
        register muSelCharPlayerArea* playerArea;
        asm
        {
            mr playerArea, r30;
        }

        u8* altRequestArea = *altRequestAreaAddrLoc;
        u32 requested = altRequestArea[playerArea->m_areaIdx];
        if (requested == altNum)
        {
            altRequestArea[playerArea->m_areaIdx] = altNone;
        }
        setCharNameFrameFromCurrAlt(playerArea);
    }
    void onSelcharInit()
    {
        register muSelCharTask* selCharTask;
        asm
        {
            mr selCharTask, r30;
        }

        buildNumAltEnabledBuffer(selCharTask);
    }
    void onSelCharUpdate()
    {
        register gmSelCharData* selCharData;
        asm
        {
            mr selCharData, r3;
        }

        //OSReport_N("%sSelCharUpdate Data @ 0x%08X\n", outputTag, selCharData);
    }
    void onPlayerAreaInit()
    {
        register muSelCharPlayerArea* playerArea;
        asm
        {
            mr playerArea, r30;
        }
           
        setCharNameFrameFromCurrAlt(playerArea);
    }
    void onPlayerAreaUpdate()
    {
        register muSelCharPlayerArea* playerArea;
        asm
        {
            lwz playerArea, 0x40(r30);
        }

        if (playerArea->m_charKind != Selch_SelectNone && playerArea->m_charKind != Selch_Random)
        {
            u8* altRequestArea = *altRequestAreaAddrLoc;

            muSelCharCoin* heldCoin = playerArea->m_selCharHand->m_selCharCoin;
            muSelCharPlayerArea* targetPlayerArea = (heldCoin == NULL) ? playerArea : heldCoin->m_selCharPlayerArea;
            u32 portNo = targetPlayerArea->m_areaIdx;
            u32 requested = altRequestArea[portNo];

            gfPadStatus padStatus;
            g_gfPadSystem->getSysPadStatus(playerArea->m_controllerNo, &padStatus);
            u32 downRZBits = (padStatus.m_buttonsCurrentFrame.bits >> 0x4) & 0b11;
            u32 justPressedRZBits = (padStatus.m_buttonsPressedThisFrame.bits >> 0x4) & 0b11;
            u32 justReleasedRZBits = (padStatus.m_buttonsReleasedThisFrame.bits >> 0x4) & 0b11;
            
            if (justPressedRZBits == 0b00 && justReleasedRZBits == 0b00) return;

            if (justPressedRZBits != 0b00)
            {
                // If we've performed NumAlts input.
                if (downRZBits == altNum)
                {
                    requested = (requested == altNum) ? altNone : altNum;
                }
                else 
                {
                    requested = downRZBits;
                }
            }
            if (justReleasedRZBits != 0b00)
            {
                if (requested != altNum && justReleasedRZBits == requested)
                {
                    requested = altNone;
                }
            }

            altRequestArea[portNo] = requested;
            setCharNameFrameFromCurrAlt(targetPlayerArea);
        }
    }
    
    // Note: SORA_MENU_SEL_CHAR .text real addr = 0x806828C4, ghidra addr = 0x8068E160, B89C
    void Init()
    {
        //SyringeCore::syInlineHook(0x80688E54, reinterpret_cast<void*>(onButtonProc));
        // 0x80688E74
        //SyringeCore::syInlineHookRel(0x65B0, reinterpret_cast<void*>(onSelCharUpdate), Modules::SORA_MENU_SEL_CHAR);

        // 0x8068363C: 0x788 bytes into symbol "initProc/[muSelCharTask]/mu_selchar.o" @ 0x80682EB4
        SyringeCore::syInlineHookRel(0xD78, reinterpret_cast<void*>(onSelcharInit), Modules::SORA_MENU_SEL_CHAR);

        // 0x80683634: 0xB24 bytes into symbol "initProc/[muSelCharTask]/mu_selchar.o" @ 0x80682EB4
        //SyringeCore::syInlineHookRel(0x1114, reinterpret_cast<void*>(onSelcharInit), Modules::SORA_MENU_SEL_CHAR);

        // 0x80693CF8: 0x334 bytes into symbol "initProc/[muSelCharPlayerArea]/mu_selchar_player_area.o" @ 0x806939C4
        SyringeCore::syInlineHookRel(0x11434, reinterpret_cast<void*>(onPlayerAreaInit), Modules::SORA_MENU_SEL_CHAR);
        // 0x80687874: 0x1A0 bytes into symbol "processDefault/[muSelCharTask]/mu_selchar.o" @ 0x806876D4
        SyringeCore::syInlineHookRel(0x4FB0, reinterpret_cast<void*>(onPlayerAreaUpdate), Modules::SORA_MENU_SEL_CHAR);
        // 0x80697100: 0x1A0 bytes into symbol "setCharKind/[muSelCharPlayerArea]/mu_selchar_player_area_" @ 0x80696F60
        SyringeCore::syInlineHookRel(0x1483C, reinterpret_cast<void*>(onSetCharKind), Modules::SORA_MENU_SEL_CHAR);
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}