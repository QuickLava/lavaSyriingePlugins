#include <sy_compat.h>
#include <modules.h>
#include <ft/fighter.h>
#include <gf/gf_archive.h>
#include "customEvents.h"

namespace customEvents
{
    const char outputTag[] = "[customEvents] ";

    // Returns the specified argument as an integer, resolving variables and casting types as necessary.
    // If the specified argument doesn't exist, or is not of Integer, Float, Boolean, or Variable type, m_errorOnValueFetch is set to 1.
    int getAsInt(soAnimCmdArgList& list, u32 index)
    {
        int result = 0;

        if (index < list.m_argList.size())
        {
            const acCmdArgConv& targetArg = list.m_argList.at(index);
            int rawValue = targetArg.data;
            switch (targetArg.argType)
            {
                case AnimCmd_Arg_Type_Int: case AnimCmd_Arg_Type_Bool: { result = rawValue; break; }
                case AnimCmd_Arg_Type_Scalar: { result = rawValue / 60000; break; }
                case AnimCmd_Arg_Type_Variable:
                {
                    u32 varMemKind = (rawValue >> 0x1C) & 0xF;
                    u32 varDataType = (rawValue >> 0x18) & 0xF;
                    if (varDataType != 2 || varMemKind == 0)
                    {
                        result = soValueAccesser::getValueInt(list.m_moduleAccesser, rawValue, 0);
                    }
                    else
                    {
                        result = list.m_moduleAccesser->getWorkManageModule().isFlag(rawValue);
                    }
                    break;
                }
                default: { list.m_errorOnValueFetch = 1; break; }
            }
            list.m_errorOnValueFetch = 0;
        }
        else
        {
            list.m_errorOnValueFetch = 1;
        }

        return result;
    };
    // Returns the specified argument as a float, resolving variables and casting types as necessary.
    // If the specified argument doesn't exist, or is not of Integer, Float, Boolean, or Variable type, m_errorOnValueFetch is set to 1.
    double getAsFloat(soAnimCmdArgList& list, u32 index)
    {
        double result = 0.0f;
        if (index < list.m_argList.size())
        {
            const acCmdArgConv& targetArg = list.m_argList.at(index);
            int rawValue = targetArg.data;
            float fltValue = rawValue;
            switch (targetArg.argType)
            {
                case AnimCmd_Arg_Type_Int: case AnimCmd_Arg_Type_Bool: { result = fltValue; break; }
                case AnimCmd_Arg_Type_Scalar: { result = fltValue / 60000.0f; break; }
                case AnimCmd_Arg_Type_Variable:
                {
                    u32 varMemKind = (rawValue >> 0x1C) & 0xF;
                    u32 varDataType = (rawValue >> 0x18) & 0xF;
                    if (varDataType != 2 || varMemKind == 0)
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
            list.m_errorOnValueFetch = 0;
        }
        else
        {
            list.m_errorOnValueFetch = 1;
        }
        return result;
    };
    // Returns the specified argument as a boolean, resolving variables and casting types as necessary.
    // If the specified argument doesn't exist, or is not of Integer, Float, Boolean, or Variable type, m_errorOnValueFetch is set to 1.
    bool getAsBool(soAnimCmdArgList& list, u32 index)
    {
        return getAsInt(list, index) != 0;
    };

    typedef u32(*notifyEventAnimCmdFnPtr)(void*, acAnimCmdImpl*, soModuleAccesser*);

    notifyEventAnimCmdFnPtr _sound_notifyEventAnimCmd;
    u32 sound_notifyEventAnimCmd(soSoundModuleImpl* moduleIn, acAnimCmdImpl* commandIn, soModuleAccesser* accesserIn)
    {
        u32 result = 0;
        u32 cmdType = commandIn->cmdAddr->type;
        u32 argCount = commandIn->cmdAddr->argNum;
        soAnimCmdArgList argList = commandIn->getArgList();
        argList.m_moduleAccesser = accesserIn;
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
                    u32 seHandleID = getAsInt(argList, 0);
                    // Initialize our modifier to 1.0f, which we'll use if no modifier argument was supplied.
                    double modifier = 1.0f;
                    // If one was supplied though...
                    if (argCount > 1)
                    {
                        // ... grab it as a float!
                        modifier = getAsFloat(argList, 1);
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
        soAnimCmdArgList argList = commandIn->getArgList();
        argList.m_moduleAccesser = accesserIn;
        switch (cmdType)
        {
            // Basic Variable Manipulation Functions
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
                u32 destinationVariableIdx = argList.getValueIndex(2);
                // If grabbing that variable ID was successful, continue.
                if (argList.m_errorOnValueFetch != 0) break;
                
                // Apply the result value...
                moduleIn->setInt(resultValue, destinationVariableIdx);
                // ... and signal that we've successfully handled the command!
                result = 1;
                break;
            }
            // Float Variable Manipulation Functions
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
                u32 destinationVariableIdx = argList.getValueIndex(2);
                // If grabbing that variable ID was successful, continue.
                if (argList.m_errorOnValueFetch != 0) break;

                // Apply the result value...
                moduleIn->setFloat(resultValue, destinationVariableIdx);
                // ... and signal that we've successfully handled the command!
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