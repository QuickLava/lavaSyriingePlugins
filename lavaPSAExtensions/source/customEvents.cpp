#include <sy_compat.h>
#include <modules.h>
#include <ft/fighter.h>
#include <gf/gf_archive.h>
#include <so/anim/so_anim_cmd_arg_list.h>
#include "customEvents.h"

namespace customEvents
{
    const char outputTag[] = "[customEvents] ";

    // Returns the specified argument as a float, resolving variables and casting types as necessary.
    // If the specified argument doesn't exist, or is not of Integer, Float, Boolean, or Variable type, m_errorOnValueFetch is set to 1.
    double getAsDouble(soAnimCmdArgList& list, u32 index)
    {
        double result = 0.0f;
        if (index < list.m_argList.size())
        {
            const acCmdArgConv& targetArg = list.m_argList.at(index);
            u32 rawValue = targetArg.data;
            result = rawValue;
            list.m_errorOnValueFetch = 0;
            switch (targetArg.argType)
            {
                case AnimCmd_Arg_Type_Int: case AnimCmd_Arg_Type_Bool: { break; }
                case AnimCmd_Arg_Type_Scalar: { result /= 60000.0; break; }
                case AnimCmd_Arg_Type_Variable:
                {
                    u32 varMemKind = (rawValue >> 0x1C) & 0xF;
                    u32 varDataType = (rawValue >> 0x18) & 0xF;
                    if (varDataType != ANIM_CMD_BOOL || varMemKind == ANIM_CMD_VAR_TYPE_IC)
                    {
                        result = soValueAccesser::getValueFloat(list.m_moduleAccesser, rawValue, 0);
                    }
                    else
                    {
                        if (list.m_moduleAccesser->getWorkManageModule().isFlag(rawValue))
                        {
                            result = 1.0f;
                        }
                    }
                    break;
                }
                default: { list.m_errorOnValueFetch = 1; break; }
            }
        }
        else
        {
            list.m_errorOnValueFetch = 1;
        }
        return result;
    };

    void setVariable(soWorkManageModuleImpl* moduleIn, u32 variableIDIn, double valueIn)
    {
        // Store the value in the destination variable according to its type!
        char varMemKind = (variableIDIn >> 0x1C) & 0xF;
        char varDataType = (variableIDIn >> 0x18) & 0xF;
        if (varMemKind != ANIM_CMD_VAR_TYPE_IC)
        {
            switch (varDataType)
            {
                case ANIM_CMD_INT: { moduleIn->setInt(int(valueIn), variableIDIn); break; }
                case ANIM_CMD_FLOAT: { moduleIn->setFloat(valueIn, variableIDIn); break; }
                case ANIM_CMD_BOOL: { moduleIn->setFlag(valueIn != 0.0f, variableIDIn); break; }
            }
        }
    }


    typedef u32(*notifyEventAnimCmdFnPtr)(void*, acAnimCmdImpl*, soModuleAccesser*);

    notifyEventAnimCmdFnPtr _sound_notifyEventAnimCmd;
    u32 sound_notifyEventAnimCmd(soSoundModuleImpl* moduleIn, acAnimCmdImpl* commandIn, soModuleAccesser* accesserIn)
    {
        u32 result = 0;
        u32 cmdType = commandIn->cmdAddr->type;
        u32 argCount = commandIn->cmdAddr->argNum;
        soAnimCmdArgList argList(accesserIn, commandIn->getArgList());
        switch (cmdType)
        {
            // Set SE Pitch  [0x0A0E0200]
            // Set SE Volume [0x0A0F0200]
            case 0xE: case 0xF:
            {
                // If we have at least one argument...
                if (argCount > 0)
                {
                    // ... fetch the handle ID as an integer.
                    u32 seHandleID = argList.getInt(0);
                    // Initialize our modifier to 1.0f, which we'll use if no modifier argument was supplied.
                    double modifier = 1.0f;
                    // If one was supplied though...
                    if (argCount > 1)
                    {
                        // ... grab it as a float!
                        modifier = argList.getFloat(1);
                    }
                    // Apply the modifier to the SE instance.
                    if (cmdType == 0xE)
                    {
                        modifier = (modifier >= 0.05f) ? modifier : 0.05f;
                        moduleIn->setSEPitch(seHandleID, modifier);
                    }
                    else
                    {
                        moduleIn->setSEVol(modifier, seHandleID, 0);
                    }
                    result = 1;
                }
                break;
            }
        }
        // If we haven't successfully resolved the command ourselves...
        if (result == 0)
        {
            // ... then pass it through to the native function and use its return value as our result.
            result = _sound_notifyEventAnimCmd((void*)moduleIn, commandIn, accesserIn);
        }
        return result;
    }
    notifyEventAnimCmdFnPtr _workManage_notifyEventAnimCmd;
    u32 workManage_notifyEventAnimCmd(soWorkManageModuleImpl* moduleIn, acAnimCmdImpl* commandIn, soModuleAccesser* accesserIn)
    {
        u32 result = 0;
        u32 cmdType = commandIn->cmdAddr->type;
        u32 argCount = commandIn->cmdAddr->argNum;
        soAnimCmdArgList argList(accesserIn, commandIn->getArgList());
        switch (cmdType)
        {
            // Basic Variable Manipulation Functions
            // [12010300] Add Basic Variable
            // [12020300] Subtract Basic Variable
            // [120D0300] Multiply Basic Variable
            // [120E0300] Divide Basic Variable
            case 0x01: case 0x02: case 0x0D: case 0x0E:
            {
                // Provided we have at least 3 arguments, continue.
                if (argCount < 3) break;

                // Initialize error value to 0.
                argList.m_errorOnValueFetch = 0;
                // The first argument will always be whatever our modifier is.
                int modifierValue = argList.getInt(0);
                // The second argument will always hold the value we apply our modifer to.
                int recipientValue = argList.getInt(1);
                // If no errors occurred while grabbing the values for our operation, continue!
                if (argList.m_errorOnValueFetch != 0) break;

                // This will hold our resulting value (which we'll initialize to our recipient's value).
                int resultValue = recipientValue;
                switch (cmdType)
                {
                    case 0x01: { resultValue = recipientValue + modifierValue; break; }
                    case 0x02: { resultValue = recipientValue - modifierValue; break; }
                    case 0x0D: { resultValue = recipientValue * modifierValue; break; }
                    case 0x0E: { if (modifierValue != 0x00) { resultValue = recipientValue / modifierValue; } break; }
                }

                // This will hold our destination variable's ID, which will be the third argument!
                u32 destinationVarIdx = argList.getValueIndex(2);
                // If grabbing that variable ID was successful, continue.
                if (argList.m_errorOnValueFetch != 0) break;
                
                // Apply the result value...
                moduleIn->setInt(resultValue, destinationVarIdx);
                // ... and signal that we've successfully handled the command!
                result = 1;
                break;
            }
            // Float Variable Manipulation Functions
            // [12070300] Add Float Variable
            // [12080300] Subtract Float Variable
            // [120F0300] Multiply Float Variable
            // [12100300] Divide Float Variable
            case 0x07: case 0x08: case 0x0F: case 0x10:
            {
                // Provided we have at least 3 arguments, continue.
                if (argCount < 3) break;

                // Initialize error value to 0.
                argList.m_errorOnValueFetch = 0;
                // The first argument will always be whatever our modifier is.
                double modifierValue = argList.getFloat(0);
                // The second argument will always hold the value we apply our modifer to.
                double recipientValue = argList.getFloat(1);
                // If no errors occurred while grabbing the values for our operation, continue!
                if (argList.m_errorOnValueFetch != 0) break;

                // This will hold our resulting value (which we'll initialize to our recipient's value).
                double resultValue = recipientValue;
                switch (cmdType)
                {
                    case 0x07: { resultValue = recipientValue + modifierValue; break; }
                    case 0x08: { resultValue = recipientValue - modifierValue; break; }
                    case 0x0F: { resultValue = recipientValue * modifierValue; break; }
                    case 0x10: { if (modifierValue != 0.0f) { resultValue = recipientValue / modifierValue; } break; }
                }

                // This will hold our destination variable's ID, which will be the third argument, enabling commands of the form VarA = VarB + VarC!
                u32 destinationVarIdx = argList.getValueIndex(2);
                // If grabbing that variable ID was successful, continue.
                if (argList.m_errorOnValueFetch != 0) break;

                // Apply the result value...
                moduleIn->setFloat(resultValue, destinationVarIdx);
                // ... and signal that we've successfully handled the command!
                result = 1;
                break;
            }
            // Aggregate Operations Functions
            // [12300300] Get Minimum Value
            // [12310300] Get Maximum Value
            // [12320300] Get Total Value
            // [12330300] Get Average Value
            case 0x30: case 0x31: case 0x32: case 0x33:
            {
                // If we've got less than 2 arguments, we can just exit, there's nothing for us to do.
                if (argCount < 2) break;

                // First, we'll grab the variable ID we'll be writing to.
                u32 destinationVarIdx = argList.getValueIndex(0);
                // If that argument wasn't a variable for some reason, then exit.
                if (argList.m_errorOnValueFetch == 1) break;

                // If the variable is and IC, exit, cuz we can't overwrite those.
                if (((destinationVarIdx >> 0x1C) & 0xF) == ANIM_CMD_VAR_TYPE_IC) break;

                // Initiate variables for the values we're tracking.
                double currMax = getAsDouble(argList, 1);
                double currMin = currMax;
                double totalValue = currMax;
                // Then, starting with the second argument, iterate through every argument...
                for (u32 i = 2; i < argCount; i++)
                {
                    // ... and fetch its value.
                    double currValue = getAsDouble(argList, i);
                    // If the current value is bigger than the current maximum...
                    if (currMax < currValue)
                    {
                        // ... overwrite it as the maximum.
                        currMax = currValue;
                    }
                    // If the current value is smaller than the current minimum...
                    if (currMin > currValue)
                    {
                        // ... then overwrite it as the minimum.
                        currMin = currValue;
                    }
                    // Lastly, add current value to total.
                    totalValue += currValue;
                }
                // Calculate our result value, based on the operation we're doing.
                double finalVal = 0.0;
                switch (cmdType)
                {
                    case 0x30: { finalVal = currMin; break; }
                    case 0x31: { finalVal = currMax; break; }
                    case 0x32: { finalVal = totalValue; break; }
                    case 0x33: { finalVal = totalValue / double(argCount - 1); break; }
                }

                // Store the result in the destination variable according to its type!
                setVariable(moduleIn, destinationVarIdx, finalVal);
                result = 1;
                break;
            }
            // [12340300] Clamp Value
            case 0x34:
            {
                // If we've got less than 3 arguments, we can just exit, there's nothing for us to do.
                if (argCount < 3) break;

                // First, we'll grab the variable ID we'll probably be writing to, since that'll have our base value.
                u32 destinationVarIdx = argList.getValueIndex(0);
                // If that argument wasn't a variable for some reason, then exit.
                if (argList.m_errorOnValueFetch == 1) break;

                // Otherwise, grab its actual value, as this is what we'll be clamping regardless.
                double resultValue = getAsDouble(argList, 0);
                double minValue = getAsDouble(argList, 1);
                double maxValue = getAsDouble(argList, 2);
                resultValue = (resultValue >= minValue) ? resultValue : minValue;
                resultValue = (resultValue <= maxValue) ? resultValue : maxValue;

                // Lastly, if we were supplied a fourth argument...
                if (argCount > 3)
                {
                    // ... and that argument is a variable ID...
                    u32 outputVariable = argList.getValueIndex(3);
                    if (argList.m_errorOnValueFetch != 1)
                    {
                        // ... we'll write our result to that instead!
                        destinationVarIdx = outputVariable;
                    }
                }

                // Store the result in the destination variable according to its type!
                setVariable(moduleIn, destinationVarIdx, resultValue);
                result = 1;
                break;
            }
        }
        // If we haven't successfully resolved the command ourselves...
        if (result == 0)
        {
            // ... then pass it through to the native function and use its return value as our result.
            result = _workManage_notifyEventAnimCmd((void*)moduleIn, commandIn, accesserIn);
        }
        return result;
    }

    void registerHooks()
    {
        // Sound Module (0xA) Events
        SyringeCompat::syReplaceFuncRel(0x56C5C, reinterpret_cast<void*>(sound_notifyEventAnimCmd), reinterpret_cast<void**>(&_sound_notifyEventAnimCmd), Modules::SORA_MELEE);
        // WorkManage Module (0x12) Events
        SyringeCompat::syReplaceFuncRel(0xA23B4, reinterpret_cast<void*>(workManage_notifyEventAnimCmd), reinterpret_cast<void**>(&_workManage_notifyEventAnimCmd), Modules::SORA_MELEE);
    }
}