#include <sy_core.h>
#include <modules.h>
#include <ac/ac_anim_cmd_impl.h>
#include <so/anim/so_anim_cmd.h>
#include <so/so_value_accesser.h>
#include "rasterizeVars.h"

namespace rasterizeVars
{
    const char outputTag[] = "[rasterizeVars] ";
    const char rasterizeFmtStr[] = "%sRasterized Arg%2d in %08X: %08X -> %08X (%s)\n";
    const u32 maxArgumentCount = 0x30;

    enum argType
    {
        at_NULL = 0x00,
        at_INT,
        at_FLT,
        at_BOL,
    };
    struct argTypeBank
    {
        argType arg01 : 2; argType arg02 : 2; argType arg03 : 2; argType arg04 : 2;
        argType arg05 : 2; argType arg06 : 2; argType arg07 : 2; argType arg08 : 2;
        argType arg09 : 2; argType arg10 : 2; argType arg11 : 2; argType arg12 : 2;
        argType arg13 : 2; argType arg14 : 2; argType arg15 : 2; argType arg16 : 2;
    public:
        argType getArgType(register int index) const
        {
            register const argTypeBank* thisptr = this;

            asm
            {
                mr r3, thisptr;
                mr r4, index;
                lwz r12, 0x00(r3);                        // Load target word!
                rlwinm r4, r4, 1, 0x1B, 0x1E;             // \ 
                addi r4, r4, 0x2;                         // / Get number of bits to shift by; ((index & 0xF) << 1) + 2...
                rlwnm r3, r12, r4, 0x1E, 0x1F;            // ... and use that to shift the desired bits into the bottom of r3!
            }
        }
    };
    enum typeLibraryIndices
    {
        tli_NULL = -1,
        tli_EMPTY,
        tli_033,
        tli_0011111113,
        tli_0011111111111113,
        tli_0001110111300111,
        tli_1111001,
        tli_01,
        tli_000,
        tli_030,
        tli_00111,
        tli_00555,
        tli_00111100,
        tli_000001111111000,
        tli_0001111111111310,
        tli_0000000011100033,
        tli_0001111000300303,
        tli_0111,
        tli_1,
        tli_111,
        tli_0000,
        tli_00000,
        tli_000011,
        tli_0,
        tli_1133,
        tli_COUNT
    };
#pragma c99 on
    const argTypeBank typeBankLibrary[tli_COUNT] =
    {
        [tli_EMPTY] = {
        },
        [tli_033] = { 
            at_INT, at_BOL, at_BOL
        },
        [tli_0011111113] = { 
            at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, // 0011111113
            at_FLT, at_BOL                                                  // 13
        },
        [tli_0011111111111113] = {
            at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, // 00111111
            at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_BOL  // 11111113
        }, 
        [tli_0001110111300111] = {
            at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_INT, at_FLT, // 00011101
            at_FLT, at_FLT, at_BOL, at_INT, at_INT, at_FLT, at_FLT, at_FLT, // 11300111
        },
        [tli_1111001] = {
            at_FLT, at_FLT, at_FLT, at_FLT, at_INT, at_INT, at_FLT          // 1111001
        },
        [tli_01] = {
            at_INT, at_FLT
        },
        [tli_000] = {
            at_INT, at_INT, at_INT
        },
        [tli_030] = {
            at_INT, at_BOL, at_INT
        },
        [tli_00111] = {
            at_INT, at_INT, at_FLT, at_FLT, at_FLT
        },
        [tli_00555] = {
            at_INT, at_INT, at_NULL, at_NULL, at_NULL
        },
        [tli_00111100] = {
            at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_INT, at_INT  // 00111100
        },
        [tli_000001111111000] = {
            at_INT, at_INT, at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, // 00000111
            at_FLT, at_FLT, at_FLT, at_FLT, at_INT, at_INT, at_INT          // 1111000
        },
        [tli_0001111111111310] = {
            at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, // 00011111
            at_FLT, at_FLT, at_FLT, at_FLT, at_BOL, at_FLT, at_INT          // 11111310
        },
        [tli_0000000011100033] = {
            at_INT, at_INT, at_INT, at_INT, at_INT, at_INT, at_INT, at_INT, // 00000000
            at_FLT, at_FLT, at_FLT, at_INT, at_INT, at_INT, at_BOL, at_BOL, // 11100033
        },
        [tli_0001111000300303] = {
            at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_INT, // 00011110
            at_INT, at_INT, at_BOL, at_INT, at_INT, at_BOL, at_INT, at_BOL, // 00300303
        },
        [tli_0111] = {
            at_INT, at_FLT, at_FLT, at_FLT
        },
        [tli_1] = {
            at_FLT
        },
        [tli_111] = {
            at_FLT, at_FLT, at_FLT
        },
        [tli_0000] = {
            at_INT, at_INT, at_INT, at_INT
        },
        [tli_00000] = {
            at_INT, at_INT, at_INT, at_INT, at_INT
        },
        [tli_000011] = {
            at_INT, at_INT, at_INT, at_INT, at_FLT, at_FLT
        },
        [tli_0] = {
            at_INT
        },
        [tli_1133] = {
            at_FLT, at_FLT, at_BOL, at_BOL
        },
    };
#pragma c99 off

    // Group 0x00: System Commands (Flow of Execution)
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_System[] =
    {
        { 0x01, tli_1 },                               // [00010100] Synchronous Timer
        { 0x02, tli_1 },                               // [00020100] Asynchronous Timer
        { 0x04, tli_0 },                               // [00040100] Set Loop
        { 0x07, tli_0 },                               // [00070100] Subroutine
        { 0x09, tli_0 },                               // [00090100] Goto
    };
    // Group 0x02: soStatusModule
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_Status[] =
    {
        { 0x06, tli_111 },                             // [02060100] Enable Transition Term
        { 0x07, tli_111 },                             // [02070200] Enable Transition Term in Transition Group
        { 0x08, tli_111 },                             // [02080100] Disable Transition Term
        { 0x09, tli_111 },                             // [02090200] Disable Transition Term in Transition Group
        { 0x0A, tli_1 },                               // [020A0100] Enable Transition Group
        { 0x0B, tli_1 },                               // [020B0100] Disable Transition Group
        { 0x0D, tli_1 },                               // [020D0100] Set Next Status Kind
        { 0x0E, tli_1 },                               // [020E0100] Set Default Target Transition Group (sets .groupID, which determines what -1 maps to!)
    };
    // Group 0x03: soModelModule
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_Model[] =
    {
        { 0x01, tli_0111 },                            // [03010400] Set Bone Rotation (XYZ)
        { 0x02, tli_01 },                              // [03020200] Set Bone Rotation (X)
        { 0x03, tli_01 },                              // [03030200] Set Bone Rotation (Y)
        { 0x04, tli_01 },                              // [03040200] Set Bone Rotation (Z)
        { 0x06, tli_0111 },                            // [03060400] Set Bone Scale (XYZ)
        { 0x07, tli_01 },                              // [03070200] Set Bone Scale (X)
        { 0x08, tli_01 },                              // [03080200] Set Bone Scale (Y)
        { 0x09, tli_01 },                              // [03090200] Set Bone Scale (Z)
        { 0x0B, tli_0111 },                            // [030B0400] Set Bone Translation (XYZ)
        { 0x0C, tli_01 },                              // [030C0200] Set Bone Translation (X)
        { 0x0D, tli_01 },                              // [030D0200] Set Bone Translation (Y)
        { 0x0E, tli_01 },                              // [030E0200] Set Bone Translation (Z)
    };
    // Group 0x04: soMotionModule
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_Motion[] =
    {
        { 0x0E, tli_01 },                              // [040E0200] Set Bone Motion Override Animation Frame
        { 0x0F, tli_01 },                              // [040F0200] Set Bone Motion Override Frame Speed Modifier
    };
    // Group 0x05: soPostureModule
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_Posture[] =
    {
        { 0x05, tli_1 },                               // [05050100] Change Model Size
        { 0x06, tli_111 },                             // [05060300] Rotate Character Model
        { 0x09, tli_111 },                             // [05090300] Set Character Position
        { 0x0A, tli_111 },                             // [050A0300] Set Character Position 2
        { 0x0B, tli_111 },                             // [050B0300] Set Character Position (Relative)
    };
    // Group 0x06: soCollision_______Modules
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_Collision[] =
    {
        { 0x00, tli_000001111111000 },                 // [06000D00] Offensive Collision
        { 0x01, tli_01 },                              // [06010200] Change Hitbox Damage
        { 0x02, tli_01 },                              // [06020200] Change Hitbox Size
        { 0x0A, tli_00111100 },                        // [060A0800] Catch Collision
        { 0x0E, tli_0000000011100033, tli_0 },         // [060E1100] Throw Attack Collision
        { 0x0F, tli_00555 },                           // [060F0500] Throw Collision
        { 0x10, tli_0001111000300303, tli_0 },         // [06101100] Inert Collision
        { 0x15, tli_000001111111000 },                 // [06150F00] Special Offensive Collision
        { 0x17, tli_000 },                             // [06170300] Defensive Collision
        { 0x1B, tli_00111 },                           // [061B0500] Move Hitbox
        { 0x1E, tli_030 },                             // [061E0300] Defensive Collision: Flip Toggle
        { 0x24, tli_0001111111111310 },                // [06241000] Generate Defensive Collision Bubble
        { 0x2B, tli_000001111111000 },                 // [062B0D00] Thrown Collision
        { 0x2C, tli_000001111111000 },                 // [062C0F00] Special Collateral Collision
    };
    // Group 0x0E: soKineticModule
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_Kinetic[] =
    {
        { 0x02, tli_0 },                               // [0E020100] Prevent Horizontal Gravity
        { 0x03, tli_0 },                               // [0E030100] ???
        { 0x04, tli_0 },                               // [0E040100] Prevent Horizontal Gravity
        { 0x05, tli_0 },                               // [0E050100] Set Air/Ground Article/Item
        { 0x06, tli_0 },                               // [0E060100] Disallow Certain Movements
        { 0x07, tli_0 },                               // [0E070100] Reallow Certain Movements
        { 0x08, tli_1133 },                            // [0E080400] Set Momentum, Set/Add Momentum
    };
    // Group 0x11: soEffectModule
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_Effect[] =
    {
        { 0x00, tli_0011111111111113 },                // [11001000] Graphic Effect
        { 0x01, tli_0011111113 },                      // [11010A00] Graphic Effect (Attached)
        { 0x02, tli_0011111113 },                      // [11020A00] Graphic Effect (Attached 2)
        { 0x03, tli_0001110111300111, tli_1111001 },   // [11031400] Sword Glow
        { 0x04, tli_0001110111300111, tli_1111001 },   // [11041700] Sword/Hammer Glow
        { 0x15, tli_033},                              // [11150300] Terminate Graphic Effect
        { 0x19, tli_0011111113 },                      // [11190A00] Graphic Effect (Attached 19)
        { 0x1A, tli_0011111111111113 },                // [111A1000] Graphic Effect (Stepping)
        { 0x1B, tli_0011111111111113 },                // [111B1000] Graphic Effect (Landing)
        { 0x1C, tli_0011111111111113 },                // [111C1000] Graphic Effect (Tumbling)
    };
    // Group 0x21: soColorBlendModule
    const cmdWhitelistEntry cmdWhitelist::m_allowedCommands_ColorBlend[] =
    {
        { 0x01, tli_0000 },                            // [21010400] Flash Overlay Effect
        { 0x02, tli_00000 },                           // [21020500] Change Flash Overlay Color
        { 0x05, tli_000011 },                          // [21050600] Flash Light Effect
        { 0x07, tli_00000 },                           // [21070500] Change Flash Light Color
    };
#pragma c99 on
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define COUNT_OF_ALIGNED(x) COUNT_OF(x) + (COUNT_OF(x) % 2)
    const cmdWhitelistCmdGroup cmdWhitelist::m_commandGroups[] =
    {
        [cmdWhitelist::scg_SYSTEM] = {cmid_SYSTEM, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_System)},
        [cmdWhitelist::scg_STATUS] = {cmid_STATUS, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_Status)},
        [cmdWhitelist::scg_MODEL] = {cmid_MODEL, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_Model)},
        [cmdWhitelist::scg_MOTION] = {cmid_MOTION, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_Motion)},
        [cmdWhitelist::scg_POSTURE] = {cmid_POSTURE, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_Posture)},
        [cmdWhitelist::scg_COLLISION] = {cmid_COLLISION, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_Collision)},
        [cmdWhitelist::scg_KINETIC] = {cmid_KINETIC, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_Kinetic)},
        [cmdWhitelist::scg_EFFECT] = {cmid_EFFECT, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_Effect)},
        [cmdWhitelist::scg_COLOR_BLEND] = {cmid_COLOR_BLEND, COUNT_OF_ALIGNED(cmdWhitelist::m_allowedCommands_ColorBlend)},
    };
    const u32 commandGroupCount = COUNT_OF(cmdWhitelist::m_commandGroups);
#pragma c99 off

    void rasterizeVariables(acAnimCmdImpl* cmdIn, soModuleAccesser* accesserIn, soAnimCmd* proxyIn, soAnimCmdArgument* argBufferIn)
    {
        soAnimCmd* rawCommandPtr = ((soAnimCmd**)cmdIn)[2];
        u32 commandSignature = *((u32*)rawCommandPtr);

        if (rawCommandPtr->m_argCount <= 0x00 || rawCommandPtr->m_argCount > maxArgumentCount) return;

        bool groupAllowed = 0;
        u32 entriesToTargetVector = 0x00;
        const cmdWhitelistCmdGroup* currGroup = cmdWhitelist::m_commandGroups - 1;
        for (u32 i = 0, targetModuleID = (commandSignature >> 0x18) & 0xFF; i < commandGroupCount; i++)
        {
            currGroup++;
            u32 currModuleID = currGroup->m_moduleID;
            if (currModuleID == targetModuleID)
            {
                groupAllowed = 1;
                break;
            }
            else
            {
                entriesToTargetVector += currGroup->m_entryCount;
                if (currModuleID > targetModuleID)
                {
                    break;
                }
            }
        }
        if (!groupAllowed) return;
        
        bool cmdAllowed = 0;
        const u32 targetCmdListCount = currGroup->m_entryCount;
        const cmdWhitelistEntry* currWhitelistEntry = cmdWhitelist::m_allowedCommands_System + entriesToTargetVector;
        for (u32 i = 0, targetCommandID = (commandSignature >> 0x10) & 0xFF; i < targetCmdListCount; i++)
        {
            u32 currCommandID = currWhitelistEntry->m_code;
            if (currCommandID == targetCommandID)
            {
                cmdAllowed = 1;
                break;
            }
            else
            {
                currWhitelistEntry++;
                if (currCommandID > targetCommandID)
                {
                    break;
                }
            }
        }
        if (!cmdAllowed) return;

        soAnimCmdArgument* currArg = rawCommandPtr->m_args;
        soAnimCmdArgument* currBufArg = argBufferIn;
        const u32 currArgCount = rawCommandPtr->m_argCount;
        for (u32 i = 0; i < currArgCount; i++)
        {
            const argTypeBank* targetFlagBank = &typeBankLibrary[currWhitelistEntry->m_bankIndices[(i >> 0x4) & 0b11]];
            argType currArgType = targetFlagBank->getArgType(i);
            if ((currArgType != at_NULL) && currArg->m_varType == AnimCmd_Arg_Type_Variable)
            {
                char varMemType = (currArg->m_rawValue >> 0x1E) & 0x3;
                char varDataType = (currArg->m_rawValue >> 0x1C) & 0x3;
                if (currArgType == at_BOL)
                {
                    currBufArg->m_varType = AnimCmd_Arg_Type_Bool;
                    if (varDataType != ANIM_CMD_BOOL || varMemType == ANIM_CMD_VAR_TYPE_IC)
                    {
                        currBufArg->m_rawValue = soValueAccesser::getValueInt(accesserIn, currArg->m_rawValue, 0) != 0;
                    }
                    else
                    {
                        currBufArg->m_rawValue = accesserIn->getWorkManageModule()->isFlag(currArg->m_rawValue);
                    }
                    OSReport_N(rasterizeFmtStr, outputTag, i, commandSignature, currArg->m_rawValue, currBufArg->m_rawValue, "BOL");
                }
                else if (currArgType == at_FLT)
                {
                    currBufArg->m_varType = AnimCmd_Arg_Type_Scalar;
                    currBufArg->m_rawValue = (int)(soValueAccesser::getValueFloat(accesserIn, currArg->m_rawValue, 0) * 60000.0f);
                    OSReport_N(rasterizeFmtStr, outputTag, i, commandSignature, currArg->m_rawValue, currBufArg->m_rawValue, "FLT");
                }
                else
                {
                    currBufArg->m_varType = AnimCmd_Arg_Type_Int;
                    currBufArg->m_rawValue = soValueAccesser::getValueInt(accesserIn, currArg->m_rawValue, 0);
                    OSReport_N(rasterizeFmtStr, outputTag, i, commandSignature, currArg->m_rawValue, currBufArg->m_rawValue, "INT");
                }
            }
            else
            {
                currBufArg->m_varType = currArg->m_varType;
                currBufArg->m_rawValue = currArg->m_rawValue;
            }
            currArg++;
            currBufArg++;
        }

        *(u32*)proxyIn = *(u32*)rawCommandPtr;
        proxyIn->m_args = argBufferIn;
        ((soAnimCmd**)cmdIn)[2] = proxyIn;
    }
    asm void rasterizeVariablesHook()
    {
        nofralloc
        mflr r31;               // Backup LR in a non-volatile register!
                                // Call main function body!
                                // param1 is curr acAnimCmdImpl*, already in r3
        lwz r4, 0x44(r30);      // param2 is soModuleAccesser*
        lwz r6, 0x00(r1);       // Grab function's native stack frame.
        addi r5, r6, 0x40;      // param3 is soAnimCmd proxy
        addi r6, r6, 0x48;      // param4 is soAnimCmdArgument array
        bl rasterizeVariables;  // Call!

        mtlr r31;			    // Restore Trampoline LR!
        blr; 				    // Return, either to Trampoline or to Pre-Trampoline!
    }

    void applyStackSpaceAdjustment(gfModuleInfo* loadedModuleIn)
    {
        gfModuleHeader* header = loadedModuleIn->m_module->header;
        if (header->id == Modules::SORA_MELEE)
        {
            // Patch "interpretNotSystemCmd/[soAnimCmdInterpreter]" to allocate stack space for each call's rasterized arg array!
            u32 textAddr = header->getTextSectionAddr(); 
            *(u32*)(textAddr + 0x705C8) = 0x9421FEC0;                   // op stwu r1, -0x140(r1)  @ $8077AFDC
            *(u32*)(textAddr + 0x705D0) = 0x90010144;                   // op stw r0, 0x144(r1)    @ $8077AFE4
            *(u32*)(textAddr + 0x708C8) = 0x80010144;                   // op lwz r0, 0x144(r1)    @ $8077B2DC
            *(u32*)(textAddr + 0x708D0) = 0x38210140;                   // op addi r1, r1, 0x140   @ $8077B2E4
            OSReport_N("%sInstalled Stack Space Adjustment!\n", outputTag);
        }
    }

    void registerHooks(CoreApi* api)
    {
        SyringeCore::ModuleLoadEvent::Subscribe(applyStackSpaceAdjustment);
        // Hook 0x8077AFF8: 0x1C bytes into symbol "interpretNotSystemCmd/[soAnimCmdInterpreter]" @ 0x8077AFDC
        api->syInlineHookRel(0x705E4, reinterpret_cast<void*>(rasterizeVariablesHook), Modules::SORA_MELEE); // 0x8077AFF8
    }
}
