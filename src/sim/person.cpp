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

const char* traitName(Trait trait) {
    switch (trait) {
    case Trait::Brave: return "Brave";
    case Trait::Timid: return "Timid";
    case Trait::Kind: return "Kind";
    case Trait::Greedy: return "Greedy";
    case Trait::Talkative: return "Talkative";
    case Trait::Diligent: return "Diligent";
    default: return "?";
    }
}

} // namespace odysseus::sim
