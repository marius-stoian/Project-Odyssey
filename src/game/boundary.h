#pragma once

// Consumer identity belongs to the compiling target, never to its dependencies.
#if (defined(ODYSSEUS_LAYER_CORE) + defined(ODYSSEUS_LAYER_PLATFORM) + defined(ODYSSEUS_LAYER_PHYSICS) + defined(ODYSSEUS_LAYER_ENGINE) + defined(ODYSSEUS_LAYER_SIM) + defined(ODYSSEUS_LAYER_GAME)) != 1
#error "Architecture include error: game/boundary.h requires exactly one source layer identity"
#endif
#if defined(ODYSSEUS_LAYER_CORE)
#error "Architecture include error: game/boundary.h is forbidden from CORE source layer"
#endif
#if defined(ODYSSEUS_LAYER_PLATFORM)
#error "Architecture include error: game/boundary.h is forbidden from PLATFORM source layer"
#endif
#if defined(ODYSSEUS_LAYER_PHYSICS)
#error "Architecture include error: game/boundary.h is forbidden from PHYSICS source layer"
#endif
#if defined(ODYSSEUS_LAYER_ENGINE)
#error "Architecture include error: game/boundary.h is forbidden from ENGINE source layer"
#endif
#if defined(ODYSSEUS_LAYER_SIM)
#error "Architecture include error: game/boundary.h is forbidden from SIM source layer"
#endif
