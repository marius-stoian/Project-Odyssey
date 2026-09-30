#include "sim/person.h"

namespace odysseus::sim {

const char* causeName(CauseOfDeath cause) {
    switch (cause) {
    case CauseOfDeath::None: return "nothing";
    case CauseOfDeath::Starvation: return "starvation";
    case CauseOfDeath::Cold: return "the cold";
    case CauseOfDeath::OldAge: return "old age";
    case CauseOfDeath::Hunting: return "a hunting accident";
    case CauseOfDeath::Childbirth: return "childbirth";
    }
    return "?";
}

} // namespace odysseus::sim
