#include <syWrapper.h>

#if __ENABLE_LEGACY_HOOK_MODE
#else
namespace SyringeCompat
{
    CoreApi* g_API = NULL;
}
#endif
