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

    soAnimCmdArgument argBuffer[maxArgumentCount];
    soAnimCmd proxyCommand = {0, 0, 0, 0, argBuffer};

    void rasterizeVariables()
    {
        register acAnimCmdImpl* cmdIn;
        register soModuleAccesser* accesserIn;
        asm
        {
            mr cmdIn, r3;
            lwz accesserIn, 0x44(r30);
        }

        soAnimCmd* rawCommandPtr = ((soAnimCmd**)cmdIn)[2];

        if (rawCommandPtr->m_argCount > maxArgumentCount)
        {
            OSReport_N(errorFmtStr, outputTag, "Too many arguments!");
            return;
        }

        soAnimCmdArgument* currArg = rawCommandPtr->m_args;
        soAnimCmdArgument* currBufArg = argBuffer;
        OSReport_N("%sRasterizing Args (%08X: 0x%08X -> 0x%08X)\n", outputTag, 
            *((u32*)rawCommandPtr), currArg, currBufArg);

        for (int i = 0; i < rawCommandPtr->m_argCount; i++)
        {
            if (currArg->m_varType == AnimCmd_Arg_Type_Variable)
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
            }
            currArg++;
            currBufArg++;
        }

        proxyCommand.m_module = rawCommandPtr->m_module;
        proxyCommand.m_code = rawCommandPtr->m_code;
        proxyCommand.m_argCount = rawCommandPtr->m_argCount;
        proxyCommand.m_option = rawCommandPtr->m_option;
        ((soAnimCmd**)cmdIn)[2] = &proxyCommand;
    }
    void registerHooks()
    {
        SyringeCore::syInlineHookRel(0x705E4, reinterpret_cast<void*>(rasterizeVariables), Modules::SORA_MELEE); // 0x8077AFF8
    }
}
