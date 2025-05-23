#ifndef LAVA_CM_ADDON_INTERFACE_H_V1
#define LAVA_CM_ADDON_INTERFACE_H_V1

#include <cstdlib>
#include "logUtils.h"

namespace codeMenu
{
    enum lineType
    {
        lt_SELECTION = 0,
        lt_INTEGER,
        lt_FLOATING,
        lt_SUB_MENU,
        lt_COMMENT,
        lt_PRINT
    };
#pragma options align= packed
    struct cmSelectionLine
    {
        u16 m_size;
        u8 m_type;
        u8 m_flags;
        u8 m_color;
        u16 m_textOffset;
        u8 m_lineNum;
        u32 m_value;
        u16 m_upOff;
        u16 m_downOff;
        u32 m_default;
        u32 m_max;
    };
    const u32 test = offsetof(cmSelectionLine, cmSelectionLine::m_value);
    struct cmIntegerLine
    {
        u16 m_size;
        u8 m_type;
        u8 m_flags;
        u8 m_color;
        u16 m_textOffset;
        u8 m_lineNum;
        u32 m_value;
        u16 m_upOff;
        u16 m_downOff;
        u32 m_default;
        u32 m_max;
        u32 m_min;
        u32 m_speed;
    };
    struct cmFloatingLine
    {
        u16 m_size;
        u8 m_type;
        u8 m_flags;
        u8 m_color;
        u16 m_textOffset;
        u8 m_lineNum;
        float m_value;
        u16 m_upOff;
        u16 m_downOff;
        float m_default;
        float m_max;
        float m_min;
        float m_speed;
    };
#pragma options align = reset

    bool loadCodeMenuAddonLOCsToBuffer(const char* addonShortName, u32* buffer, u32 entryCount);
}

#endif
