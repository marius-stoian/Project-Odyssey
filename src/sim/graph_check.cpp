#include "sim/graph_check.h"

#include "sim/needs.h"
#include "sim/rule_effect.h"

#include <algorithm>
#include <cctype>
#include <deque>
#include <format>
#include <functional>

namespace odysseus::sim::rules {

namespace {

using Kind = GraphFinding::Kind;

bool isNeedName(const std::string& name) {
    const auto lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    };
    for (std::size_t i = 0; i < kNeedCount; ++i) {
        if (lower(name) == lower(needName(static_cast<Need>(i)))) return true; // the game reads need names in any case
    }
    return false;
}

struct Checker {
    const GraphCatalog& catalog;
    std::string file;
    std::vector<GraphFinding> findings;

    void add(Kind kind, bool error, const std::string& key, const std::string& message) {
        findings.push_back({kind, error, file, key, message});
    }

    // The names an expression asks about: has(...) items, need(...) needs, tag(...) tags.
    void expression(const Expr& e, const std::string& key, const std::string& where) {
        if (e.kind == Expr::Kind::Call) {
            const auto word = [&](std::size_t i) -> const Expr* {
                return i < e.args.size() && e.args[i] && e.args[i]->kind == Expr::Kind::Text ? e.args[i].get() : nullptr;
            };
            if (e.text == "need") {
                if (const Expr* w = word(0); w != nullptr && !isNeedName(w->text)) add(Kind::UnknownNeed, true, key, std::format("{}: unknown need \"{}\"", where, w->text));
            } else if (e.text == "has") {
                const Expr* w = word(e.args.size() == 3 ? 1 : 0);
                if (w != nullptr && !catalog.items.empty() && catalog.items.count(w->text) == 0) {
                    add(Kind::UnknownItem, true, key, std::format("{}: unknown item \"{}\"", where, w->text));
                }
            } else if (e.text == "tag") {
                if (const Expr* w = word(1); w != nullptr && !catalog.tags.empty() && catalog.tags.count(w->text) == 0) {
                    add(Kind::UnknownTag, false, key, std::format("{}: no thing carries the tag \"{}\"", where, w->text));
                }
            }
        }
        for (const ExprPtr& arg : e.args) {
            if (arg) expression(*arg, key, where);
        }
    }

    void source(const std::string& text, const std::string& key, const std::string& where) {
        if (text.empty()) return;
        const ParsedExpr parsed = parseExpression(text);
        if (parsed.root) expression(*parsed.root, key, where);
    }

    void effect(const Effect& e, const std::string& key, const std::string& where) {
        const auto& verbs = knownVerbs();
        const bool known = std::any_of(verbs.begin(), verbs.end(), [&](const VerbInfo& v) { return e.verb == v.name; });
        if (!known) {
            add(Kind::UnknownVerb, true, key, std::format("{}: unknown effect \"{}\"", where, e.verb));
            return;
        }
        const auto arg = [&](std::size_t i) { return i < e.argSources.size() ? e.argSources[i] : std::string(); };
        if ((e.verb == "give" || e.verb == "take") && !catalog.items.empty() && !arg(1).empty() && catalog.items.count(arg(1)) == 0) {
            add(Kind::UnknownItem, true, key, std::format("{}: unknown item \"{}\"", where, arg(1)));
        } else if (e.verb == "do" && !catalog.builtins.empty() && !arg(0).empty() && catalog.builtins.count(arg(0)) == 0) {
            add(Kind::UnknownBuiltin, true, key, std::format("{}: unknown built-in action \"{}\"", where, arg(0)));
        } else if (e.verb == "talk" && !catalog.dialogues.empty() && !arg(0).empty() && catalog.dialogues.count(arg(0)) == 0) {
            add(Kind::UnknownDialogue, true, key, std::format("{}: unknown conversation \"{}\"", where, arg(0)));
        } else if (e.verb == "start" && !catalog.interactions.empty() && !arg(0).empty() && catalog.interactions.count(arg(0)) == 0) {
            add(Kind::UnknownInteraction, true, key, std::format("{}: unknown interaction \"{}\"", where, arg(0)));
        }
        for (const ExprPtr& a : e.args) {
            if (a) expression(*a, key, where);
        }
        if (e.inner) effect(*e.inner, key, where);
    }
};

} // namespace

std::vector<GraphFinding> checkDialogue(const DlgScript& script, const GraphCatalog& catalog) {
    Checker c{catalog, script.file, {}};
    // Reachability: breadth-first from the start node.
    std::set<std::string> seen;
    if (const DlgNode* start = script.startNode()) {
        std::deque<const DlgNode*> queue{start};
        seen.insert(start->id);
        while (!queue.empty()) {
            const DlgNode* node = queue.front();
            queue.pop_front();
            for (const DlgChoice& choice : node->choices) {
                const DlgNode* next = choice.target == "END" ? nullptr : script.find(choice.target);
                if (next != nullptr && seen.insert(next->id).second) queue.push_back(next);
            }
        }
    }
    const bool oneLine = !script.bark.empty() || script.pair.size() == 2; // greetings and pair talks have no choices by design
    for (const DlgNode& node : script.nodes) {
        if (seen.count(node.id) == 0) c.add(Kind::Unreachable, false, node.id, std::format("node \"{}\" cannot be reached: no choice leads to it", node.id));
        if (node.choices.empty() && !oneLine) {
            c.add(Kind::DeadEnd, true, node.id, std::format("node \"{}\" is a dead end: it has no choice and no END", node.id));
        }
        for (std::size_t i = 0; i < node.lines.size(); ++i) {
            c.source(node.lines[i].conditionSource, std::format("{}/line{}/if", node.id, i), std::format("{}, line {}", node.id, i + 1));
        }
        for (std::size_t i = 0; i < node.choices.size(); ++i) {
            const DlgChoice& choice = node.choices[i];
            const std::string where = std::format("{}, choice {}", node.id, i + 1);
            const std::string key = std::format("{}/choice{}", node.id, i);
            if (choice.target != "END" && script.find(choice.target) == nullptr) {
                c.add(Kind::UnknownNode, true, key, std::format("{}: unknown node \"{}\"", where, choice.target));
            }
            c.source(choice.conditionSource, key + "/if", where);
            for (const Effect& e : choice.effects) c.effect(e, key + "/do", where);
        }
    }
    return std::move(c.findings);
}

std::vector<GraphFinding> checkInteraction(const Interaction& i, const GraphCatalog& catalog) {
    Checker c{catalog, i.file.empty() ? "interactions/" + i.id + ".json" : i.file, {}};
    for (std::size_t k = 0; k < i.requires_.size(); ++k) c.source(i.requires_[k].source, std::format("requirement{}", k), std::format("requirement {}", k + 1));
    for (const Effect& e : i.effects) c.effect(e, "effects0", "effects");
    if (i.npc) c.source(i.npc->scoreSource, "npcrule", "NPC rule");
    if (!catalog.tags.empty()) {
        for (const std::string& tag : i.targetTags) {
            if (catalog.tags.count(tag) == 0) c.add(Kind::UnknownTag, false, "target", std::format("target: no thing carries the tag \"{}\"", tag));
        }
    }
    return std::move(c.findings);
}

} // namespace odysseus::sim::rules
