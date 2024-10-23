#include <sy_core.h>
#include <modules.h>
#include <ac/ac_anim_cmd_impl.h>
#include <so/anim/so_anim_cmd.h>
#include <so/so_value_accesser.h>
#include "rasterizeVars.h"

namespace rasterizeVars
{
    const char outputTag[] = "[rasterizeVars] ";
    const char rasterizeFmtStr[] = "%s- Rasterized Arg #%d (%s): %08X -> %08X\n";
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
        argType arg17 : 2; argType arg18 : 2; argType arg19 : 2; argType arg20 : 2;
        argType arg21 : 2; argType arg22 : 2; argType arg23 : 2; argType arg24 : 2;
        argType arg25 : 2; argType arg26 : 2; argType arg27 : 2; argType arg28 : 2;
        argType arg29 : 2; argType arg30 : 2; argType arg31 : 2; argType arg32 : 2;
    public:
        argType getArgType(register int index) const
        {
            register const argTypeBank* thisptr = this;

            asm
            {
                mr r3, thisptr;
                mr r4, index;
                rlwinm r12, r4, 30, 0x1D, 0x1D;           // Get offset to target word; offset = (index > 0x10) ? 0x4 : 0x00;
                lwzx r12, r3, r12;                        // Load target word!
                rlwinm r4, r4, 1, 0x1B, 0x1E;             // \ 
                addi r4, r4, 0x2;                         // / Get number of bits to shift by; ((index & 0xF) << 1) + 2...
                rlwnm r3, r12, r4, 0x1E, 0x1F;            // ... and use that to shift the desired bits into the bottom of r3!
            }
        }
    };
    enum typeLibraryIndices
    {
        fli_NULL = -1,
        fli_033,
        fli_0011111113,
        fli_0011111111111113,
        fli_00011101113001111111001,
        fli_COUNT
    };
    const argTypeBank typeBankLibrary[fli_COUNT] =
    {
        // 033
        { at_INT, at_BOL, at_BOL },
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
        // 00011101113001111111001
        {
            at_INT, at_INT, at_INT, at_FLT, at_FLT, at_FLT, at_INT, at_FLT, // 00011101
            at_FLT, at_FLT, at_BOL, at_INT, at_INT, at_FLT, at_FLT, at_FLT, // 11300111
            at_FLT, at_FLT, at_FLT, at_FLT, at_INT, at_INT, at_FLT          // 1111001
        },
    };

    struct cmdWhitelistEntry
    {
        u8 m_module; // AnimCMD Instruction Module ID
        u8 m_code; // AnimCMD Instruction Code ID
        u8 m_option; // AnimCMD Instruction Option ID
        u8 m_bankIndex; // flagLibraryIndices entry identifying the associated flagBank
    };
    const cmdWhitelistEntry allowedCommands[] =
    {
        { 0x11, 0x15, 0x00, fli_033},                      // [11150300] Terminate Graphic Effect
        { 0x11, 0x01, 0x00, fli_0011111113 },              // [11010A00] Graphic Effect (Attached)
        { 0x11, 0x02, 0x00, fli_0011111113 },              // [11020A00] Graphic Effect (Attached 2)
        { 0x11, 0x19, 0x00, fli_0011111113 },              // [11190A00] Graphic Effect (Attached 19)
        { 0x11, 0x00, 0x00, fli_0011111111111113 },        // [11001000] Graphic Effect
        { 0x11, 0x1A, 0x00, fli_0011111111111113 },        // [111A1000] Graphic Effect (Stepping)
        { 0x11, 0x1B, 0x00, fli_0011111111111113 },        // [111B1000] Graphic Effect (Landing)
        { 0x11, 0x1C, 0x00, fli_0011111111111113 },        // [111C1000] Graphic Effect (Tumbling)
        { 0x11, 0x03, 0x00, fli_00011101113001111111001 }, // [11031400] Sword Glow
        { 0x11, 0x04, 0x00, fli_00011101113001111111001 }, // [11041700] Sword/Hammer Glow
    };
    const u32 allowedCommandCount = sizeof(allowedCommands) / sizeof(cmdWhitelistEntry);

    void rasterizeVariables(acAnimCmdImpl* cmdIn, soModuleAccesser* accesserIn, soAnimCmd* proxyIn, soAnimCmdArgument* argBufferIn)
    {
        soAnimCmd* rawCommandPtr = ((soAnimCmd**)cmdIn)[2];

        bool cmdAllowed = 0;
        const cmdWhitelistEntry* currWhitelistEntry = allowedCommands - 1;
        for (int i = 0; !cmdAllowed && i < allowedCommandCount; i++)
        {
            currWhitelistEntry++;
            if (currWhitelistEntry->m_module == rawCommandPtr->m_module)
            {
                cmdAllowed = (currWhitelistEntry->m_code == 0xFFul) || (currWhitelistEntry->m_code == rawCommandPtr->m_code);
            }
        }
        if (!cmdAllowed) return;

        OSReport_N("%sRasterizing Args (%08X: ", outputTag, *((u32*)rawCommandPtr));
        if (rawCommandPtr->m_argCount > maxArgumentCount)
        {
            OSReport_N("Skipping, too many arguments!\n");
            return;
        }
        else if (rawCommandPtr->m_argCount == 0x00)
        {
            OSReport_N("Skipping, no arguments!\n");
            return;
        }
        
        soAnimCmdArgument* currArg = rawCommandPtr->m_args;
        soAnimCmdArgument* currBufArg = argBufferIn;
        OSReport_N("0x%08X -> 0x%08X)\n", currArg, currBufArg);

        const argTypeBank* targetFlagBank = &typeBankLibrary[currWhitelistEntry->m_bankIndex];

        for (int i = 0; i < rawCommandPtr->m_argCount; i++)
        {
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
                    OSReport_N(rasterizeFmtStr, outputTag, i, "Bool", currArg->m_rawValue, currBufArg->m_rawValue);
                }
                else if (currArgType == at_FLT)
                {
                    currBufArg->m_varType = AnimCmd_Arg_Type_Scalar;
                    currBufArg->m_rawValue = (int)(soValueAccesser::getValueFloat(accesserIn, currArg->m_rawValue, 0) * 60000.0f);
                    OSReport_N(rasterizeFmtStr, outputTag, i, "Scalar", currArg->m_rawValue, currBufArg->m_rawValue);
                }
                else
                {
                    currBufArg->m_varType = AnimCmd_Arg_Type_Int;
                    currBufArg->m_rawValue = soValueAccesser::getValueInt(accesserIn, currArg->m_rawValue, 0);
                    OSReport_N(rasterizeFmtStr, outputTag, i, "Int", currArg->m_rawValue, currBufArg->m_rawValue);
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

        proxyIn->m_module = rawCommandPtr->m_module;
        proxyIn->m_code = rawCommandPtr->m_code;
        proxyIn->m_argCount = rawCommandPtr->m_argCount;
        proxyIn->m_option = rawCommandPtr->m_option;
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

    void registerHooks()
    {
        SyringeCore::syInlineHookRel(0x705E4, reinterpret_cast<void*>(rasterizeVariablesHook), Modules::SORA_MELEE); // 0x8077AFF8
    }
}
