#include "hum/PackEntryImpl.h"

#include "PackManifestJson.h"

namespace hum { void hum_register_pack_core(Registry&); }

HUM_DEFINE_PACK_ENTRY(hum::hum_register_pack_core, kPackManifestJson)
