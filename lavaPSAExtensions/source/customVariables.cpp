#include <cstdlib>
#include <sy_core.h>
#include <modules.h>
#include <ft/fighter.h>
#include <so/so_module_accesser.h>
#include "customVariables.h"

namespace customVars
{
    const char outputTag[] = "[customVars] ";
    enum customVariableIDs
    {
        ft_Dynamic_Ef_Bank_ID = 30100,
        ft_Dynamic_Per_Costume_Ef_Bank_ID,
    };

    u32 intVarIntercept(soModuleAccesser* moduleAccesser, u32 variableID, u32 originalResult)
    {
        u32 result = originalResult;

        if (moduleAccesser->m_stageObject->m_taskCategory == gfTask::Category_Fighter)
        {
            Fighter* fighterPtr = (Fighter*)moduleAccesser->m_stageObject;
            switch (variableID)
            {
            case ft_Dynamic_Ef_Bank_ID:
            {
                result = fighterPtr->getFtKind() + 0x43B7;
                result = result << 0x10;
                break;
            }
            case ft_Dynamic_Per_Costume_Ef_Bank_ID:
            {
                result = (fighterPtr->getFtKind() * 0x80) + 0x3B7;
                result += moduleAccesser->getWorkManageModule()->getInt(Fighter::Instance_Work_Int_Color);
                result = result << 0x10;
                break;
            }
            default:
            {
                break;
            }
            }
        }
        
        if (result != originalResult)
        {
            OSReport_N("%sBasic Var Intercept [ID: %d, Val: 0x%02X -> 0x%02X]\n", outputTag, variableID, originalResult, result);
        }

        return result;
    }
    asm void intVarInterceptHook()
    {
        nofralloc
        mflr r31;                     // Backup LR in a non-volatile register!
                                      // Call main function body!
        mr r5, r3;                    // Move original result into r5!
        mr r3, r29;                   // Get moduleAccesser from r29...
        mr r4, r30;                   // ... and variableID from r30!
        bl intVarIntercept;            // Call!
        stw r3, 0x08(r1);             // Write processed result over r3 backup on the stack!
                                      // Additionally, we need to restore the function's overwritten LR value from r0.
        lwz r11, 0x00(r1);            // Grab the address of the main function's stack frame...
        lwz r12, 0x24(r11);           // ... use it to grab the LR value for the main function...
        stw r12, 0x4(r11);            // ... and store it over the LR backed up by the trampoline!
        mtlr r31;                     // Restore LR...
        blr;                          // ... and return!
    }
    void registerHooks()
    {
        // Int Variable Intercept Hook @ 0x807972B8: 0x1B4 bytes into symbol "getValueInt/[soValueAccesser]/so_value_accesser.o"
        SyringeCore::syInlineHookRel(0x8C8A4, reinterpret_cast<void*>(intVarInterceptHook), Modules::SORA_MELEE);
    }
}