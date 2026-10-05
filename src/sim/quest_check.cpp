#include "sim/graph_check.h"

#include <algorithm>
#include <deque>
#include <format>
#include <map>
#include <set>

namespace odysseus::sim::rules {

namespace {

using Kind = GraphFinding::Kind;

struct QuestChecker {
    const Quest& quest;
    const GraphCatalog& catalog;
    std::vector<GraphFinding> findings;

    void add(Kind kind, bool error, const std::string& key, const std::string& message) {
        findings.push_back({kind, error, quest.file.empty() ? "quests/" + quest.id + ".json" : quest.file, key, message});
    }

    // quest(id), step(id) and has(item) inside a condition.
    void expression(const Expr& e, const std::string& key, const std::string& where) {
        if (e.kind == Expr::Kind::Call) {
            const auto word = [&](std::size_t i) -> const Expr* { return i < e.args.size() && e.args[i] && e.args[i]->kind == Expr::Kind::Text ? e.args[i].get() : nullptr; };
            if ((e.text == "quest" || e.text == "step") && word(0) != nullptr) {
                if (!catalog.quests.empty() && catalog.quests.count(word(0)->text) == 0) add(Kind::UnknownQuest, true, key, std::format("{}: unknown quest \"{}\"", where, word(0)->text));
            } else if (e.text == "has") {
                const Expr* w = word(e.args.size() == 3 ? 1 : 0);
                if (w != nullptr && !catalog.items.empty() && catalog.items.count(w->text) == 0) add(Kind::UnknownItem, true, key, std::format("{}: unknown item \"{}\"", where, w->text));
            }
        }
        for (const ExprPtr& arg : e.args) {
            if (arg) expression(*arg, key, where);
        }
    }

    void objective(const QuestStep& step) {
        const std::string key = "step:" + step.id;
        const QuestObjective& o = step.objective;
        const auto unknown = [&](Kind kind, bool error, const std::set<std::string>& known, const char* what) {
            if (!known.empty() && known.count(o.subject) == 0) add(kind, error, key, std::format("step {}: unknown {} \"{}\"", step.id, what, o.subject));
        };
        switch (o.kind) {
        case QuestObjective::Kind::Gather:
        case QuestObjective::Kind::Give:
        case QuestObjective::Kind::Craft: unknown(Kind::UnknownItem, true, catalog.items, "item"); break;
        case QuestObjective::Kind::Interact: unknown(Kind::UnknownInteraction, true, catalog.interactions, "interaction"); break;
        case QuestObjective::Kind::Talk: unknown(Kind::UnknownPerson, false, catalog.people, "person"); break; // people of a run are made at run time: a warning
        case QuestObjective::Kind::Goto: unknown(Kind::UnknownPlace, false, catalog.places, "place"); break;
        case QuestObjective::Kind::Defeat: unknown(Kind::UnknownKind, false, catalog.kinds, "kind of creature"); break;
        case QuestObjective::Kind::Wait:
        case QuestObjective::Kind::Flag: break;
        }
    }

    void reachability() {
        std::map<std::string, std::vector<std::string>> next;
        for (const QuestStep& s : quest.steps) {
            next[s.id].push_back(s.next);
            for (const QuestBranch& b : s.branches) next[s.id].push_back(b.to);
        }
        std::set<std::string> reached;
        std::deque<std::string> queue;
        if (quest.find(quest.start) != nullptr) {
            reached.insert(quest.start);
            queue.push_back(quest.start);
        }
        bool canEnd = false;
        while (!queue.empty()) {
            const std::string id = queue.front();
            queue.pop_front();
            for (const std::string& to : next[id]) {
                if (to == kQuestEnd) canEnd = true;
                else if (quest.find(to) != nullptr && reached.insert(to).second) queue.push_back(to);
            }
        }
        for (const QuestStep& s : quest.steps) {
            if (reached.count(s.id) == 0) add(Kind::Unreachable, false, "step:" + s.id, std::format("step {} is never reached: no step or branch leads to it", s.id));
        }
        if (!canEnd) {
            add(Kind::CannotFinish, true, "quest", "no step leads to END: the quest can never be finished");
            return;
        }
        // Steps from which END cannot be reached (a loop with no way out): grow the set of steps that do lead to END.
        std::set<std::string> leadsToEnd;
        bool grew = true;
        while (grew) {
            grew = false;
            for (const QuestStep& s : quest.steps) {
                if (leadsToEnd.count(s.id) != 0) continue;
                for (const std::string& to : next[s.id]) {
                    if (to == kQuestEnd || leadsToEnd.count(to) != 0) {
                        leadsToEnd.insert(s.id);
                        grew = true;
                        break;
                    }
                }
            }
        }
        for (const QuestStep& s : quest.steps) {
            if (reached.count(s.id) != 0 && leadsToEnd.count(s.id) == 0) add(Kind::CannotFinish, true, "step:" + s.id, std::format("step {} is a dead end: from here the quest can never end", s.id));
        }
    }

    void run() {
        for (const QuestCondition& c : quest.requires_) {
            if (c.expr) expression(*c.expr, "quest", "requires");
        }
        for (const QuestCondition& c : quest.fail) {
            if (c.expr) expression(*c.expr, "quest", "fail");
        }
        for (const Effect& reward : quest.rewards) {
            if ((reward.verb == "give" || reward.verb == "take") && reward.args.size() == 3 && reward.args[1]->kind == Expr::Kind::Text) {
                if (!catalog.items.empty() && catalog.items.count(reward.args[1]->text) == 0) {
                    add(Kind::UnknownItem, true, "quest", std::format("reward \"{}\": unknown item \"{}\"", reward.source, reward.args[1]->text));
                }
            } else if (reward.verb == "quest" && reward.args.size() == 2 && reward.args[1]->kind != Expr::Kind::Number) {
                const std::string& id = reward.args[1]->text;
                if (!catalog.quests.empty() && catalog.quests.count(id) == 0) add(Kind::UnknownQuest, true, "quest", std::format("reward \"{}\": unknown quest \"{}\"", reward.source, id));
            }
        }
        for (const QuestStep& s : quest.steps) {
            objective(s);
            for (const QuestBranch& b : s.branches) {
                if (b.condition.expr) expression(*b.condition.expr, "step:" + s.id, "branch to " + b.to);
            }
        }
        reachability();
    }
};

// The quests a quest needs to be done (or active): quest(x) inside requires, not under `not`.
void needs(const Expr& e, bool negated, std::set<std::string>& out) {
    if (e.kind == Expr::Kind::Call && (e.text == "quest" || e.text == "step") && !e.args.empty() && e.args[0] && e.args[0]->kind != Expr::Kind::Number && !negated) out.insert(e.args[0]->text);
    const bool inner = negated || (e.kind == Expr::Kind::Unary && e.text == "not");
    for (const ExprPtr& arg : e.args) {
        if (arg) needs(*arg, inner, out);
    }
}

} // namespace

std::vector<GraphFinding> checkQuest(const Quest& quest, const GraphCatalog& catalog) {
    QuestChecker checker{quest, catalog, {}};
    checker.run();
    return std::move(checker.findings);
}

std::vector<GraphFinding> checkQuests(const std::vector<Quest>& quests, const GraphCatalog& catalog) {
    std::vector<GraphFinding> all;
    std::map<std::string, std::set<std::string>> edges;
    std::map<std::string, const Quest*> byId;
    for (const Quest& q : quests) {
        for (GraphFinding& f : checkQuest(q, catalog)) all.push_back(std::move(f));
        byId[q.id] = &q;
        for (const QuestCondition& c : q.requires_) {
            if (c.expr) needs(*c.expr, false, edges[q.id]);
        }
    }
    // Depth-first search for a way back to the quest we started from; each cycle is reported once, from its smallest id.
    std::set<std::string> reported;
    for (const auto& entry : edges) {
        const std::string& start = entry.first;
        std::vector<std::string> path;
        std::set<std::string> onPath;
        std::set<std::string> done;
        bool found = false;
        std::vector<std::string> cycle;
        const auto visit = [&](auto&& self, const std::string& id) -> void {
            if (found || done.count(id) != 0) return;
            path.push_back(id);
            onPath.insert(id);
            for (const std::string& to : edges[id]) {
                if (to == start) {
                    found = true;
                    cycle = path;
                    return;
                }
                if (onPath.count(to) == 0) self(self, to);
                if (found) return;
            }
            onPath.erase(id);
            path.pop_back();
            done.insert(id);
        };
        visit(visit, start);
        if (!found) continue;
        std::vector<std::string> sorted = cycle;
        std::sort(sorted.begin(), sorted.end());
        if (sorted.front() != start || !reported.insert(sorted.front()).second) continue;
        std::string text;
        for (const std::string& id : cycle) text += id + " -> ";
        text += start;
        const Quest* owner = byId[start];
        all.push_back({Kind::Cycle, true, owner != nullptr && !owner->file.empty() ? owner->file : "quests/" + start + ".json", "quest", "quests need each other: " + text});
    }
    return all;
}

} // namespace odysseus::sim::rules
