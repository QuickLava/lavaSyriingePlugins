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
    const u32 numAltsEnabledBufferWordCount = maxChars / 0x20;
   u32 numAltsEnabledBuffer[numAltsEnabledBufferWordCount];

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
    const u32 test = sizeof(PAT0Header);
    static_assert(test == 0x38, "Class is wrong size!");
    
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

    //93373620
    bool buildNumAltEnabledBuffer(muSelCharTask* selCharTask)
    {
        bool result = 0;

        register nw4r::g3d::AnmObjTexPatRes* charNumPat0 = selCharTask->m_selCharPlayerAreas[0]->m_muCharName->m_modelAnim->m_anmObjTexPatRes;
        OSReport_N("%sPat0 Obj and File Address: %08X, %08X\n", outputTag, (u32)charNumPat0, (u32)charNumPat0->m_anmTexPatFile);

        PAT0Header* pat0HeaderPtr = (PAT0Header*)charNumPat0->m_anmTexPatFile;
        char* pat0BodyPtr = (char*)pat0HeaderPtr + pat0HeaderPtr->dataOffset;

        PAT0MaterialEntry* matStructPtr = (PAT0MaterialEntry*)(pat0BodyPtr + *(s32*)(pat0BodyPtr + 0x24));
        PAT0KeyframeTableHeader* matTablePtr = (PAT0KeyframeTableHeader*)((char*)matStructPtr + matStructPtr->tableOffset);
        u32 frameCount = matTablePtr->textureCount;
        PAT0Keyframe* currFrame = (PAT0Keyframe*)matTablePtr;

        u32 currBufferIndex = 0x00;
        u32 currBufferWord = 0x00000000;
        OSReport_N("%sNUMALT_BUFFER @ %08X:\n", outputTag, numAltsEnabledBuffer);
        for (int i = 0; i < frameCount; i++)
        {
            currFrame++;
            u32 frameInt = (u32)currFrame->frame;
            u32 targetCharacter = frameInt / 10;
            u32 altIndex = (frameInt - (targetCharacter * 10));

            u32 bufferIndex = targetCharacter >> 0x05;
            if (bufferIndex >= numAltsEnabledBufferWordCount) break;
            if (bufferIndex != currBufferIndex)
            {
                numAltsEnabledBuffer[currBufferIndex] = currBufferWord;
                OSReport_N("%s- %08X\n", outputTag, currBufferWord);
                currBufferWord = 0x00;
                currBufferIndex = bufferIndex;
            }
            if (altIndex == 0x4)
            {
                currBufferWord |= 1 << (targetCharacter & 0x1F);
                //OSReport_N("%sFrame %4X: CharID = %2X, AltID = %2X\n", outputTag, frameInt, targetCharacter, altIndex);
            }
        }
        for (currBufferIndex; currBufferIndex < numAltsEnabledBufferWordCount; currBufferIndex++)
        {
            numAltsEnabledBuffer[currBufferIndex] = currBufferWord;
            OSReport_N("%s- %08X (Fill)\n", outputTag, currBufferWord);
            currBufferWord = 0x00;
        }

        return result;
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

    enum requestedAlt
    {
        altNone = 0,
        altZ = 1,
        altR = 2,
        altNum = 3,
    };
    void setCharNameFrameFromCurrAlt(muSelCharPlayerArea* playerArea)
    {
        if (playerArea->m_charKind != MuSelch_SelectNone)
        {
            u8* altRequestArea = *altRequestAreaAddrLoc;

            u32 portNo = playerArea->m_areaIdx;
            MuObject* charName = playerArea->m_muCharName;
            
            u32 requested = altRequestArea[portNo];

            u32 charNameFrame = muMenu::exchangeMuSelchkind2MuStockchkind(playerArea->m_charKind);
            charNameFrame = (muMenu::exchangeGmCharacterKind2Something(charNameFrame) * 10) + 1;
            charName->setFrameTex(charNameFrame + requested);

            OSReport_N("%sPlayerArea Port No %d: CharName Frame: %d\n", outputTag, portNo, charNameFrame + requested);
        }
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

        if (playerArea->m_charKind != MuSelch_SelectNone && playerArea->m_charKind != MuSelch_Random)
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
                    u32 charID = muMenu::exchangeMuSelchkind2MuStockchkind(targetPlayerArea->m_charKind);
                    charID = muMenu::exchangeGmCharacterKind2Something(charID);
                    u32 bufferWord = numAltsEnabledBuffer[charID >> 0x05];
                    if (bufferWord & (1 << (charID & 0x1F)))
                    {
                        requested = (requested == altNum) ? altNone : altNum;
                    }
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

        // 0x80683634: 0x780 bytes into symbol "initProc/[muSelCharTask]/mu_selchar.o" @ 0x80682EB4
        SyringeCore::syInlineHookRel(0xD70, reinterpret_cast<void*>(onSelcharInit), Modules::SORA_MENU_SEL_CHAR);

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