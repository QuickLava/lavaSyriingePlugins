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
    const u32 maxArgumentCount = 0x10;

    //soAnimCmdArgument argBuffer[maxArgumentCount];
    //soAnimCmd proxyCommand = {0, 0, 0, 0, argBuffer};

    void rasterizeVariables(acAnimCmdImpl* cmdIn, soModuleAccesser* accesserIn, soAnimCmd* proxyIn, soAnimCmdArgument* argBufferIn)
    {
        soAnimCmd* rawCommandPtr = ((soAnimCmd**)cmdIn)[2];

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
            currBufArg->m_varType = currArg->m_varType;
            currBufArg->m_rawValue = currArg->m_rawValue;

            /*if (currArg->m_varType == AnimCmd_Arg_Type_Variable)
            {
                char varDataType = (currArg->m_rawValue >> 0x1C) & 0x3;
                if (varDataType == ANIM_CMD_INT)
                {
                    currBufArg->m_varType = AnimCmd_Arg_Type_Int;
                    currBufArg->m_rawValue = soValueAccesser::getValueInt(accesserIn, currArg->m_rawValue, 0);
                }
                else if (varDataType == ANIM_CMD_FLOAT)
                {
                    currBufArg->m_varType = AnimCmd_Arg_Type_Scalar;
                    currBufArg->m_rawValue = (int)(soValueAccesser::getValueFloat(accesserIn, currArg->m_rawValue, 0) * 60000.0f);
                }
                else
                {
                    currBufArg->m_varType = currArg->m_varType;
                    currBufArg->m_rawValue = currArg->m_rawValue;
                }
            }
            else
            {
                currBufArg->m_varType = currArg->m_varType;
                currBufArg->m_rawValue = currArg->m_rawValue;
            }*/
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
        mflr r31;			    // Backup LR in a non-volatile register!
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
