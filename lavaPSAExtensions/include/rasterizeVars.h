#ifndef LAVA_RASTERIZE_VARS_H_V1
#define LAVA_RASTERIZE_VARS_H_V1

#include "logUtils.h"

namespace rasterizeVars
{
    enum cmdModuleID
    {
        cmid_SYSTEM = 0x00,
        cmid_01,
        cmid_STATUS,
        cmid_MODEL,
        cmid_MOTION,
        cmid_POSTURE,
        cmid_COLLISION,
        cmid_CONTROLLER,
        cmid_GROUND,
        cmid_SITUATION,
        cmid_SOUND,
        cmid_VISIBILITY,
        cmid_CHAR_SPECIFIC,
        cmid_ANIMCMD,
        cmid_KINETIC,
        cmid_LINK,
        cmid_ARTICLE,
        cmid_EFFECT,
        cmid_WORK_MANAGE,
        cmid_COMBO,
        cmid_AREA,
        cmid_TERRITORY,
        cmid_SEARCH,
        cmid_PHYSICS,
        cmid_SLOPE,
        cmid_SHADOW,
        cmid_CAMERA,
        cmid_1B,
        cmid_STOP,
        cmid_SHAKE,
        cmid_DAMAGE,
        cmid_ITEM,
        cmid_TURN,
        cmid_COLOR_BLEND,
        cmid_TEAM,
        cmid_SLOW,
        cmid__COUNT,
    };

    struct cmdWhitelistEntry
    {
        u8 m_code; // AnimCMD Instruction Code ID
        u8 m_bankIndices[3]; // typeLibraryIndices entry identifying the typeBank for Args 0-15 and 16-32 respectively.
    };
    struct cmdWhitelistCmdGroup
    {
        u8 m_moduleID;
        u8 m_entryCount;
    };
    struct cmdWhitelist
    {
        enum supportedCommandGroups
        {
            scg_SYSTEM,
            scg_STATUS,
            scg_MODEL,
            scg_MOTION,
            scg_POSTURE,
            scg_COLLISION,
            scg_KINETIC,
            scg_EFFECT,
            scg_COLOR_BLEND,
            scg__COUNT,
        };

        static const cmdWhitelistEntry m_allowedCommands_System[];
        static const cmdWhitelistEntry m_allowedCommands_Status[];
        static const cmdWhitelistEntry m_allowedCommands_Model[];
        static const cmdWhitelistEntry m_allowedCommands_Motion[];
        static const cmdWhitelistEntry m_allowedCommands_Posture[];
        static const cmdWhitelistEntry m_allowedCommands_Collision[];
        static const cmdWhitelistEntry m_allowedCommands_Kinetic[];
        static const cmdWhitelistEntry m_allowedCommands_Effect[];
        static const cmdWhitelistEntry m_allowedCommands_ColorBlend[];
        static const cmdWhitelistCmdGroup m_commandGroups[scg__COUNT];
    };

    void registerHooks(CoreApi* api);
}

#endif
