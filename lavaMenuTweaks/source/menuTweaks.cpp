#include <sy_core.h>
#include <modules.h>
#include <logUtils.h>
#include <gf/gf_pad_system.h>
#include <gm/gm_sel_char_data.h>
#include <mu/selchar/mu_selchar_player_area.h>

namespace lavaMenuTweaks {

    const char outputTag[] = "[lavaMenuTweaks] ";

    u8** altRequestAreaAddrLoc = (u8**)0x8084CE38;
    enum requestedAlt
    {
        altNone = 0,
        altZ = 1,
        altR = 2,
        altNum = 3,
    };

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
        altRequestArea[playerArea->m_areaIdx] = 0x00;
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
            gfPadStatus padStatus;
            g_gfPadSystem->getSysPadStatus(playerArea->m_controllerNo, &padStatus);
            u32 downRZBits = (padStatus.m_buttonsCurrentFrame.bits >> 0x4) & 0b11;
            u32 justPressedRZBits = (padStatus.m_buttonsPressedThisFrame.bits >> 0x4) & 0b11;
            if (justPressedRZBits != 0b00)
            {
                u8* altRequestArea = *altRequestAreaAddrLoc;

                muSelCharCoin* heldCoin = playerArea->m_selCharHand->m_selCharCoin;
                muSelCharPlayerArea* targetPlayerArea = (heldCoin == NULL) ? playerArea : heldCoin->m_selCharPlayerArea;

                u32 portNo = targetPlayerArea->m_areaIdx;
                MuObject* charName = targetPlayerArea->m_muCharName;

                u32 requested = altRequestArea[portNo];
                requested = (requested != downRZBits) ? downRZBits : altNone;

                u32 charNameFrame = muMenu::exchangeMuSelchkind2MuStockchkind(targetPlayerArea->m_charKind);
                charNameFrame = (muMenu::exchangeGmCharacterKind2Something(charNameFrame) * 10) + 1;

                altRequestArea[portNo] = requested;
                OSReport_N("%sPlayerArea Port No %d: CharName Frame: %d\n", outputTag, portNo, charNameFrame + requested);
                charName->setFrameTex(charNameFrame + requested);
            }
        }
    }
    
    // Note: SORA_MENU_SEL_CHAR .text real addr = 0x806828C4, ghidra addr = 0x8068E160, B89C
    void Init()
    {
        //SyringeCore::syInlineHook(0x80688E54, reinterpret_cast<void*>(onButtonProc));
        // 0x80688E74
        //SyringeCore::syInlineHookRel(0x65B0, reinterpret_cast<void*>(onSelCharUpdate), Modules::SORA_MENU_SEL_CHAR);

        // 0x80687874
        SyringeCore::syInlineHookRel(0x4FB0, reinterpret_cast<void*>(onPlayerAreaUpdate), Modules::SORA_MENU_SEL_CHAR);
        // 0x80697100
        SyringeCore::syInlineHookRel(0x1483C, reinterpret_cast<void*>(onSetCharKind), Modules::SORA_MENU_SEL_CHAR);
    }

    void Destroy()
    {
        OSReport("Goodbye\n");
    }
}