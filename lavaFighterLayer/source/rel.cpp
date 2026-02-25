#include <gf/gf_file_io_request.h>
#include <gf/gf_memory_pool.h>
#include <sy_compat.h>

#include "fighterLayer.h"

namespace Syringe
{
    SY_COMPAT_EXTERN_C_BLOCK;

    const PluginMeta META = {
    "ft_ike_UpBTweaks",
    "QuickLava",
    Version("0.0.1"),
    Version(SYRINGE_VERSION),
    &main,
    .FLAGS = {
            .timing = 0,
            .loading = LOAD_PERSIST,
            .heap = Heaps::Syringe
    }};

    SY_COMPAT_PROLOG_FN(fighterLayer::Init);
    
    SY_COMPAT_MAIN_FN(fighterLayer::Init);

    SY_COMPAT_EPILOG_FN;

    SY_COMPAT_UNRESOLVED_FN;
}