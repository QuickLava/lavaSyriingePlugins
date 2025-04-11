#ifndef LAVA_LOG_UTILS_H_V1
#define LAVA_LOG_UTILS_H_V1

#include <types.h>
#include <os/OSError.h>
#include <BrawlHeaders/OpenRVL/include/revolution/macros.h>

#define __LOG_UTILS_ALLOW_LOGGING true
#if __LOG_UTILS_ALLOW_LOGGING
// Logging Enabled (via logUtils.h)!
#define OSReport_N OSReport
#else
// Logging Disabled (via logUtils.h)!
#define OSReport_N (void)sizeof
#endif

#endif
