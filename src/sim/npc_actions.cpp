#include "sim/npc_actions.h"

#include <utility>

namespace odysseus::sim {

void ClassActionSource::collect(const SourceContext& context, std::vector<ActionCandidate>& out) const {
    for (const std::string& id : context.profile.classActions) out.push_back({id, ActionOrigin::Class, 0, false, 0, 0, {}});
}

void CustomActionSource::collect(const SourceContext& context, std::vector<ActionCandidate>& out) const {
    for (const std::string& id : context.profile.customActions) out.push_back({id, ActionOrigin::Custom, 0, false, 0, 0, {}});
}

// An event on offer to this NPC (its class matches, it is near enough): the event's action, at the event.
void EventActionSource::collect(const SourceContext& context, std::vector<ActionCandidate>& out) const {
    if (context.board == nullptr || context.events == nullptr || context.board->posted().empty()) return;
    for (const EventOffer& offer : context.board->offersFor(*context.events, context.profile.classes, context.x, context.y, context.tick)) {
        out.push_back({offer.def->action, ActionOrigin::Event, offer.def->bonus, true, offer.x, offer.y, offer.def->trigger});
    }
}

ActionSources ActionSources::standard() {
    ActionSources sources;
    sources.add(std::make_unique<ClassActionSource>());
    sources.add(std::make_unique<CustomActionSource>());
    sources.add(std::make_unique<EventActionSource>());
    return sources;
}

std::vector<ActionCandidate> ActionSources::collect(const SourceContext& context) const {
    std::vector<ActionCandidate> out;
    for (const std::unique_ptr<ActionSource>& source : sources_) source->collect(context, out);
    return out;
}

} // namespace odysseus::sim
