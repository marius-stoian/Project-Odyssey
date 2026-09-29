#pragma once

// Consumer identity belongs to the compiling target, never to its dependencies.
#if (defined(ODYSSEUS_LAYER_CORE) + defined(ODYSSEUS_LAYER_PLATFORM) + defined(ODYSSEUS_LAYER_ENGINE) + defined(ODYSSEUS_LAYER_SIM) + defined(ODYSSEUS_LAYER_GAME)) != 1
#error "Architecture include error: core/boundary.h requires exactly one source layer identity"
#endif
