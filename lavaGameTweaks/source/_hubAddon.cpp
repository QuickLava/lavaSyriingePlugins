#include "_hubAddon.h"

namespace hubAddon
{
	u32 indexBuffer[lid__COUNT] = {};

	const char outputTag[] = "[lavaMechHub] ";
	const char addonShortName[] = "MECH_HUB";

	void populate()
	{
		bool initSuccessful = codeMenu::loadCodeMenuAddonLOCsToBuffer(addonShortName, indexBuffer, lid__COUNT);
		if (initSuccessful)
		{
			OSReport_N("%sSuccessfully Loaded Addon Index File to Buffer 0x%08X!\n", outputTag, indexBuffer);
		}
		else
		{
			OSReport_N("%sFailed to Load Addon Index File to Buffer!\n", outputTag);
		}
	}
	u32 getActiveMechanic()
	{
		u32 result = amid_NONE;

		codeMenu::cmSelectionLine* activeMechanicLine = (codeMenu::cmSelectionLine*)indexBuffer[lid_ACTIVE_MECHANIC];
		if (activeMechanicLine != NULL)
		{
			result = activeMechanicLine->m_value;
		}

		return result;
	}
}
