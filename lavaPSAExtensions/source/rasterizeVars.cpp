#include <sy_core.h>
#include <modules.h>
#include <ac/ac_anim_cmd_impl.h>
#include <so/anim/so_anim_cmd.h>
#include <so/so_value_accesser.h>
#include "rasterizeVars.h"

namespace rasterizeVars
{
    const char outputTag[] = "[rasterizeVars] ";
    const char errorFmtStr[] = "%sRasterization Failed: %s\n";
    const char rasterizeFmtStr[] = "%s- Rasterized Arg #%d (%s): %08X -> %08X\n";
    const u32 maxArgumentCount = 0x10;

    struct cmdWhitelistEntry
    {
        u8 m_module; // AnimCMD Instruction Module ID
        u8 m_code; // AnimCMD Instruction Code ID
        u16 m_argMask; // Bits correspond (from low to high) to each instruction argument: 0 == Skip, 1 == Evaluate
        u16 m_boolMask; // Bits correspond (from low to high) to each argument: 1 == Expect Bool
        u16 m_intFltMask; // Bits correspond (from low to high) to each instruction argument: 0 == Expect Int, 1 == Expect Flt
    };
    const cmdWhitelistEntry allowedCommands[] =
    {
        {0x11, 0xFF, 0xFFFF, 0x0000, ~0b11}, // Generic Graphic Effect Entry
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
        for (int i = 0; i < rawCommandPtr->m_argCount; i++)
        {
            if ((currWhitelistEntry->m_argMask & (0b1 << i)) && currArg->m_varType == AnimCmd_Arg_Type_Variable)
            {
                char varMemType = (currArg->m_rawValue >> 0x1E) & 0x3;
                char varDataType = (currArg->m_rawValue >> 0x1C) & 0x3;
                if (currWhitelistEntry->m_boolMask & (0b1 << i))
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
                else if (currWhitelistEntry->m_intFltMask & (0b1 << i))
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
