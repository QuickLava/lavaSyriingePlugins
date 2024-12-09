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
    const u32 maxArgumentCount = 0x20;

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
    const argTypeBank typeBankLibrary[tli_COUNT] =
    {
        // EMPTY
        {
        },
        // 033
        { 
            at_INT, at_BOL, at_BOL
        },
        // 0011111113
        { 
            at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, // 0011111113
            at_FLT, at_BOL                                                  // 13
        },
        // 0011111111111113
        {
            at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, // 00111111
            at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, at_BOL  // 11111113
        }, 
        // 0001110111300111
        {
            at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_INT, at_FLT, // 00011101
            at_FLT, at_FLT, at_BOL, at_INT, at_INT, at_FLT, at_FLT, at_FLT, // 11300111
        },
        // 1111001
        {
            at_FLT, at_FLT, at_FLT, at_FLT, at_INT, at_INT, at_FLT          // 1111001
        },
        // 01
        {
            at_INT, at_FLT
        },
        // 000
        {
            at_INT, at_INT, at_INT
        },
        // 030
        {
            at_INT, at_BOL, at_INT
        },
        // 00111
        {
            at_INT, at_INT, at_FLT, at_FLT, at_FLT
        },
        // 00555
        {
            at_INT, at_INT, at_NULL, at_NULL, at_NULL
        },
        // 00111100
        {
            at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_INT, at_INT  // 00111100
        },
        // 000001111111000
        {
            at_INT, at_INT, at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, // 00000111
            at_FLT, at_FLT, at_FLT, at_FLT, at_INT, at_INT, at_INT          // 1111000
        },
        // 0001111111111310
        {
            at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_FLT, // 00011111
            at_FLT, at_FLT, at_FLT, at_FLT, at_BOL, at_FLT, at_INT          // 11111310
        },
        // 0000000011100033
        {
            at_INT, at_INT, at_INT, at_INT, at_INT, at_INT, at_INT, at_INT, // 00000000
            at_FLT, at_FLT, at_FLT, at_INT, at_INT, at_INT, at_BOL, at_BOL, // 11100033
        },
        // 0001111000300303
        {
            at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_FLT, at_INT, // 00011110
            at_INT, at_INT, at_BOL, at_INT, at_INT, at_BOL, at_INT, at_BOL, // 00300303
        },
        // 0111
        {
            at_INT, at_FLT, at_FLT, at_FLT
        },
        // 1
        {
            at_FLT
        },
        // 111
        {
            at_FLT, at_FLT, at_FLT
        },
        // 0000
        {
            at_INT, at_INT, at_INT, at_INT
        },
        // 00000
        {
            at_INT, at_INT, at_INT, at_INT, at_INT
        },
        // 000011
        {
            at_INT, at_INT, at_INT, at_INT, at_FLT, at_FLT
        },
        // 0
        {
            at_INT
        },
        // 1133
        {
            at_FLT, at_FLT, at_BOL, at_BOL
        },
    };

    struct cmdWhitelistEntry
    {
        u8 m_module; // AnimCMD Instruction Module ID
        u8 m_code; // AnimCMD Instruction Code ID
        u8 m_bankIndices[2]; // typeLibraryIndices entry identifying the typeBank for Args 0-15 and 16-32 respectively.

        u16 getFullID() const
        {
            return *(u16*)this;
        }
    };
    const cmdWhitelistEntry allowedCommands[] =
    {
        // Graphics Commands
        { 0x11, 0x15, tli_033},                              // [11150300] Terminate Graphic Effect
        { 0x11, 0x01, tli_0011111113 },                      // [11010A00] Graphic Effect (Attached)
        { 0x11, 0x02, tli_0011111113 },                      // [11020A00] Graphic Effect (Attached 2)
        { 0x11, 0x19, tli_0011111113 },                      // [11190A00] Graphic Effect (Attached 19)
        { 0x11, 0x00, tli_0011111111111113 },                // [11001000] Graphic Effect
        { 0x11, 0x1A, tli_0011111111111113 },                // [111A1000] Graphic Effect (Stepping)
        { 0x11, 0x1B, tli_0011111111111113 },                // [111B1000] Graphic Effect (Landing)
        { 0x11, 0x1C, tli_0011111111111113 },                // [111C1000] Graphic Effect (Tumbling)
        { 0x11, 0x03, tli_0001110111300111, tli_1111001 },   // [11031400] Sword Glow
        { 0x11, 0x04, tli_0001110111300111, tli_1111001 },   // [11041700] Sword/Hammer Glow

        // Hitbox Commands
        { 0x06, 0x01, tli_01 },                              // [06010200] Change Hitbox Damage
        { 0x06, 0x02, tli_01 },                              // [06020200] Change Hitbox Size
        { 0x06, 0x17, tli_000 },                             // [06170300] Defensive Collision
        { 0x06, 0x1E, tli_030 },                             // [061E0300] Defensive Collision: Flip Toggle
        { 0x06, 0x1B, tli_00111 },                           // [061B0500] Move Hitbox
        { 0x06, 0x0F, tli_00555 },                           // [060F0500] Throw Collision
        { 0x06, 0x0A, tli_00111100 },                        // [060A0800] Catch Collision
        { 0x06, 0x00, tli_000001111111000 },                 // [06000D00] Offensive Collision
        { 0x06, 0x2B, tli_000001111111000 },                 // [062B0D00] Thrown Collision
        { 0x06, 0x15, tli_000001111111000 },                 // [06150F00] Special Offensive Collision
        { 0x06, 0x2C, tli_000001111111000 },                 // [062C0F00] Special Collateral Collision
        { 0x06, 0x24, tli_0001111111111310 },                // [06241000] Generate Defensive Collision Bubble
        { 0x06, 0x0E, tli_0000000011100033, tli_0 },         // [060E1100] Throw Attack Collision
        { 0x06, 0x10, tli_0001111000300303, tli_0 },         // [06101100] Inert Collision

        // Bone Commands
        { 0x03, 0x02, tli_01 },                              // [03020200] Set Bone Rotation (X)
        { 0x03, 0x03, tli_01 },                              // [03030200] Set Bone Rotation (Y)
        { 0x03, 0x04, tli_01 },                              // [03040200] Set Bone Rotation (Z)
        { 0x03, 0x07, tli_01 },                              // [03070200] Set Bone Scale (X)
        { 0x03, 0x08, tli_01 },                              // [03080200] Set Bone Scale (Y)
        { 0x03, 0x09, tli_01 },                              // [03090200] Set Bone Scale (Z)
        { 0x03, 0x0C, tli_01 },                              // [030C0200] Set Bone Translation (X)
        { 0x03, 0x0D, tli_01 },                              // [030D0200] Set Bone Translation (Y)
        { 0x03, 0x0E, tli_01 },                              // [030E0200] Set Bone Translation (Z)
        { 0x03, 0x01, tli_0111 },                            // [03010400] Set Bone Rotation (XYZ)
        { 0x03, 0x06, tli_0111 },                            // [03060400] Set Bone Scale (XYZ)
        { 0x03, 0x0B, tli_0111 },                            // [030B0400] Set Bone Translation (XYZ)

        // Char Pos/Rot Commands
        { 0x05, 0x05, tli_1 },                               // [05050100] Change Model Size
        { 0x05, 0x06, tli_111 },                             // [05060300] Rotate Character Model
        { 0x05, 0x09, tli_111 },                             // [05090300] Set Character Position
        { 0x05, 0x0A, tli_111 },                             // [050A0300] Set Character Position 2
        { 0x05, 0x0B, tli_111 },                             // [050B0300] Set Character Position (Relative)

        // Flash Overlay Commands
        { 0x21, 0x01, tli_0000 },                            // [21010400] Flash Overlay Effect
        { 0x21, 0x02, tli_00000 },                           // [21020500] Change Flash Overlay Color
        { 0x21, 0x07, tli_00000 },                           // [21070500] Change Flash Light Color
        { 0x21, 0x05, tli_000011 },                          // [21050600] Flash Light Effect

        // Kinetic Module Commands
        { 0x0E, 0x02, tli_0 },                               // [0E020100] Prevent Horizontal Gravity
        { 0x0E, 0x03, tli_0 },                               // [0E030100] ???
        { 0x0E, 0x04, tli_0 },                               // [0E040100] Prevent Horizontal Gravity
        { 0x0E, 0x05, tli_0 },                               // [0E050100] Set Air/Ground Article/Item
        { 0x0E, 0x06, tli_0 },                               // [0E060100] Disallow Certain Movements
        { 0x0E, 0x07, tli_0 },                               // [0E070100] Reallow Certain Movements
        { 0x0E, 0x08, tli_1133 },                            // [0E080400] Set Momentum, Set/Add Momentum

        // Anim Override Commands
        { 0x04, 0x0E, tli_01 },                              // [040E0200] Set Bone Motion Override Animation Frame
        { 0x04, 0x0F, tli_01 },                              // [040F0200] Set Bone Motion Override Frame Speed Modifier
    };
    const u32 allowedCommandCount = sizeof(allowedCommands) / sizeof(cmdWhitelistEntry);

    void rasterizeVariables(acAnimCmdImpl* cmdIn, soModuleAccesser* accesserIn, soAnimCmd* proxyIn, soAnimCmdArgument* argBufferIn)
    {
        soAnimCmd* rawCommandPtr = ((soAnimCmd**)cmdIn)[2];
        u32 commandSignature = *((u32*)rawCommandPtr);
        const u16 commandFullID = commandSignature >> 0x10;


        if (rawCommandPtr->m_argCount <= 0x00 || rawCommandPtr->m_argCount > maxArgumentCount) return;

        bool cmdAllowed = 0;
        const cmdWhitelistEntry* currWhitelistEntry = allowedCommands - 1;
        for (int i = 0; i < allowedCommandCount; i++)
        {
            currWhitelistEntry++;
            if (currWhitelistEntry->getFullID() == commandFullID)
            {
                cmdAllowed = 1;
                break;
            }
        }
        if (!cmdAllowed) return;
        
        soAnimCmdArgument* currArg = rawCommandPtr->m_args;
        soAnimCmdArgument* currBufArg = argBufferIn;

        for (int i = 0; i < rawCommandPtr->m_argCount; i++)
        {
            const argTypeBank* targetFlagBank = &typeBankLibrary[currWhitelistEntry->m_bankIndices[(i >> 0x4) & 0b1]];
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

    void registerHooks()
    {
        SyringeCore::ModuleLoadEvent::Subscribe(applyStackSpaceAdjustment);
        // Hook 0x8077AFF8: 0x1C bytes into symbol "interpretNotSystemCmd/[soAnimCmdInterpreter]" @ 0x8077AFDC
        SyringeCore::syInlineHookRel(0x705E4, reinterpret_cast<void*>(rasterizeVariablesHook), Modules::SORA_MELEE); // 0x8077AFF8
    }
}
