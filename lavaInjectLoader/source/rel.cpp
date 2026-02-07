#include <gf/gf_file_io_request.h>
#include <gf/gf_memory_pool.h>
#include <sy_compat.h>

#include "injectLoader.h"

namespace Syringe
{
    SY_COMPAT_EXTERN_C_BLOCK;

    SY_COMPAT_PLG_META("lavaInjectLoader", "QuickLava", "0.7.0", SYRINGE_VERSION);

    SY_COMPAT_PROLOG_FN(lavaInjectLoader::Init);
    
    SY_COMPAT_MAIN_FN(lavaInjectLoader::Init);

    SY_COMPAT_EPILOG_FN;

    SY_COMPAT_UNRESOLVED_FN;
}