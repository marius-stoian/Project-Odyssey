// US-304 File watch for outside edits: the watcher notices a file saved outside the game (after a short wait so a double write counts once), ignores the
// game's own writes once, never overwrites a level the Editor has unsaved changes in, and costs very little per tick.
#include "camp.h"

#include "game/data_reload.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <format>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace camp_support;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// Moves a file's modification time forward, so a test never has to wait for the clock of the file system.
void touch(const fs::path& file, int seconds = 5) {
    std::error_code error;
    fs::last_write_time(file, fs::last_write_time(file, error) + std::chrono::seconds(seconds), error);
    REQUIRE_FALSE(error);
}

void rewrite(const fs::path& file, const std::string& text, int seconds = 5) {
    writeText(file, text);
    touch(file, seconds);
}

std::string replaced(std::string text, const std::string& from, const std::string& to) {
    const std::size_t at = text.find(from);
    REQUIRE_MESSAGE(at != std::string::npos, from);
    return text.replace(at, from.size(), to);
}

struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Studio(const std::string& name) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        game::saveLevel(level, definitions, data / "watch-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "watch-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
        odyssey->setWatching(true); // after the first tick: the game's own clock never mixes with the test's
    }
    game::OdysseyGame& game() { return *odyssey; }
    // Polls at 0.25 s steps from `from` for `seconds`, and gathers what the sets did.
    std::vector<game::ReloadOutcome> run(double from, double seconds) {
        std::vector<game::ReloadOutcome> all;
        for (double t = from; t <= from + seconds; t += 0.05) {
            for (game::ReloadOutcome& outcome : odyssey->pollFiles(t)) all.push_back(std::move(outcome));
        }
        return all;
    }
};

std::string evidenceFolder() {
#ifdef _MSC_VER
    char* value = nullptr;
    std::size_t size = 0;
    std::string folder;
    if (_dupenv_s(&value, &size, "ODYSSEUS_EVIDENCE_DIR") == 0 && value != nullptr) folder = value;
    std::free(value);
    return folder;
#else
    const char* value = std::getenv("ODYSSEUS_EVIDENCE_DIR");
    return value != nullptr ? value : "";
#endif
}

} // namespace

TEST_CASE("US-304 Watcher: a changed file is reported once after the wait, a double write counts once") {
    const fs::path root = fs::temp_directory_path() / "odysseus-us304-watcher";
    fs::remove_all(root);
    fs::create_directories(root / "sub");
    writeText(root / "a.json", "1");
    writeText(root / "sub" / "b.json", "1");
    writeText(root / "sub" / "c.json.tmp", "1"); // temporary files are never reported
    game::FileWatcher watcher;
    watcher.watch(root);
    watcher.snapshot();
    CHECK(watcher.files() == 2);
    CHECK(watcher.poll(0.0).empty()); // nothing changed

    rewrite(root / "a.json", "2");
    CHECK(watcher.poll(0.30).empty());                  // seen, but it must be quiet first
    rewrite(root / "a.json", "22", 10);                 // written again: the wait starts over
    CHECK(watcher.poll(0.60).empty());                  // seen again, 0 s of quiet
    const auto settled = watcher.poll(0.95);            // 0.35 s of quiet
    REQUIRE(settled.size() == 1);
    CHECK(settled[0].filename() == "a.json");
    CHECK(watcher.poll(1.25).empty());                  // reported once, not again

    rewrite(root / "sub" / "c.json.tmp", "2");
    rewrite(root / "sub" / "b.json", "2");
    fs::remove(root / "a.json");                        // a file that went counts too
    writeText(root / "sub" / "new.json", "x");          // and one that came
    CHECK(watcher.poll(1.55).empty());
    const auto later = watcher.poll(1.90);
    CHECK(later.size() == 3);
    fs::remove_all(root);
}

TEST_CASE("US-304 Polls: the watcher looks at most four times a second") {
    const fs::path root = fs::temp_directory_path() / "odysseus-us304-polls";
    fs::remove_all(root);
    fs::create_directories(root);
    writeText(root / "a.json", "1");
    game::FileWatcher watcher;
    watcher.watch(root);
    watcher.snapshot();
    CHECK(watcher.poll(0.0).empty());
    rewrite(root / "a.json", "2");
    CHECK(watcher.poll(0.10).empty()); // too soon: not even looked at
    CHECK(watcher.poll(0.20).empty());
    CHECK(watcher.poll(0.30).empty()); // looked at now; the wait starts
    CHECK(watcher.poll(0.45).empty());
    CHECK(watcher.poll(0.65).size() == 1);
    fs::remove_all(root);
}

TEST_CASE("US-304 Own writes: a file the game wrote itself is ignored once, a later outside change is not") {
    const fs::path root = fs::temp_directory_path() / "odysseus-us304-own";
    fs::remove_all(root);
    fs::create_directories(root);
    writeText(root / "a.json", "1");
    game::FileWatcher watcher;
    watcher.watch(root);
    watcher.snapshot();
    rewrite(root / "a.json", "2");
    watcher.noteOwnWrite(root / "a.json"); // the Editor saved it
    CHECK(watcher.poll(0.30).empty());
    CHECK(watcher.poll(0.80).empty()); // never reported
    rewrite(root / "a.json", "3", 10);  // the owner edits it outside afterwards
    CHECK(watcher.poll(1.10).empty());
    CHECK(watcher.poll(1.50).size() == 1);
    // resync: the game read the root itself, and what waited under it is dropped.
    rewrite(root / "a.json", "4", 15);
    CHECK(watcher.poll(2.00).empty());
    watcher.resync(root);
    CHECK(watcher.poll(2.60).empty());
    fs::remove_all(root);
}

TEST_CASE("US-304 Outside: an interaction file saved in a text editor is live within a second, without F5") {
    Studio studio("us304-outside");
    const fs::path gather = studio.data / "interactions" / "gather.json";
    REQUIRE(studio.game().interactions().find("gather") != nullptr);
    CHECK(studio.game().interactions().find("gather")->rangeMilli == 2000);
    rewrite(gather, replaced(readText(gather), "\"range\": 2,", "\"range\": 3,"));
    const auto outcomes = studio.run(0.0, 0.9); // under a second of the watcher's own time
    REQUIRE_FALSE(outcomes.empty());
    CHECK(outcomes[0].set == "interactions");
    CHECK(outcomes[0].result.ok);
    CHECK(studio.game().interactions().find("gather")->rangeMilli == 3000); // no F5
    CHECK(studio.game().toast() == "Reloaded interactions");
    // A mistake is named and the last good data stays.
    rewrite(gather, replaced(readText(gather), "\"range\": 3,", "\"range\": 3,,"), 10);
    const auto broken = studio.run(1.0, 0.9);
    REQUIRE_FALSE(broken.empty());
    CHECK_FALSE(broken[0].result.ok);
    CHECK(studio.game().interactions().find("gather")->rangeMilli == 3000);
    CHECK_FALSE(studio.game().interactionReport().errors.empty());
}

TEST_CASE("US-304 Outside: several files saved together reload each set once") {
    Studio studio("us304-together");
    rewrite(studio.data / "interactions" / "gather.json", readText(studio.data / "interactions" / "gather.json"));
    rewrite(studio.data / "dialogue" / "elder-fire.dlg", readText(studio.data / "dialogue" / "elder-fire.dlg"));
    rewrite(studio.data / "light" / "lights.json", readText(studio.data / "light" / "lights.json"));
    const auto outcomes = studio.run(0.0, 0.9);
    std::vector<std::string> sets;
    for (const game::ReloadOutcome& outcome : outcomes) sets.push_back(outcome.set);
    CHECK(sets == std::vector<std::string>{"interactions", "lights"}); // the interactions and the dialogue are one set, read once
}

TEST_CASE("US-304 Own writes: what the Editor saves is not read a second time") {
    Studio studio("us304-editor-save");
    game::Editor& editor = studio.game().editor();
    // A graph save: the Editor writes the file and reads the set itself (US-303); the watcher sees the new time and says nothing.
    game::GraphEditor& graphs = editor.graphs();
    graphs.show(true);
    graphs.showKind(game::GraphEditor::Kind::Dialogue);
    REQUIRE(graphs.open("elder-fire"));
    graphs.addCard("comment"); // a change, so Save has something to write
    REQUIRE(graphs.save());
    CHECK(studio.run(0.0, 1.2).empty());
    // The level: the Editor saves it and the watcher keeps quiet.
    graphs.show(false);
    editor.setTool(game::EditorTool::Brush);
    REQUIRE(editor.save());
    CHECK(studio.run(2.0, 1.2).empty());
}

TEST_CASE("US-304 Open level: changed outside while the Editor has unsaved changes, nothing is overwritten and the status line says so") {
    Studio studio("us304-level");
    game::Editor& editor = studio.game().editor();
    const std::string before = editor.level().name;
    // An unsaved change: a rename of the level is the shortest edit.
    editor.setLevelName("Edited in the Editor");
    REQUIRE(editor.unsaved());
    // Meanwhile the file is changed by a text editor.
    const fs::path file = studio.data / "watch-level.json";
    rewrite(file, replaced(readText(file), "\"name\": \"" + before + "\"", "\"name\": \"Edited outside\""));
    studio.run(0.0, 0.9);
    CHECK(editor.level().name == "Edited in the Editor"); // nothing was overwritten
    CHECK(editor.unsaved());
    CHECK(editor.status().find("changed on disk") != std::string::npos);
    CHECK(studio.game().toast().find("unsaved changes are kept") != std::string::npos);
}

TEST_CASE("US-304 Open level: changed outside while the Editor has nothing unsaved, it is read again") {
    Studio studio("us304-level-clean");
    game::Editor& editor = studio.game().editor();
    REQUIRE_FALSE(editor.unsaved());
    const std::string before = editor.level().name;
    const fs::path file = studio.data / "watch-level.json";
    rewrite(file, replaced(readText(file), "\"name\": \"" + before + "\"", "\"name\": \"Edited outside\""));
    studio.run(0.0, 0.9);
    CHECK(editor.level().name == "Edited outside");
    CHECK_FALSE(editor.unsaved());
    CHECK(studio.game().toast() == "Reloaded watch-level.json");
}

TEST_CASE("US-304 Off: without watching, nothing reads a file by itself; F5 still does") {
    Studio studio("us304-off");
    studio.game().setWatching(false);
    CHECK_FALSE(studio.game().watching());
    const fs::path gather = studio.data / "interactions" / "gather.json";
    rewrite(gather, replaced(readText(gather), "\"range\": 2,", "\"range\": 3,"));
    for (int tick = 0; tick < 60; ++tick) studio.game().update({}); // the game's own ticks never poll
    CHECK(studio.game().interactions().find("gather")->rangeMilli == 2000);
    luna::engine::Intents f5;
    f5.set(luna::engine::Intent::Reload, true, true);
    studio.game().update(f5);
    CHECK(studio.game().interactions().find("gather")->rangeMilli == 3000);
}


TEST_CASE("US-304 Cost: the watcher costs a small part of a millisecond in a tick") {
    Studio studio("us304-cost");
    // The game ticks 20 times a second; the watcher looks at a handful of files in each tick and at all of them within about half a second.
    constexpr double kTick = 0.05;
    constexpr int kTicks = 400; // twenty seconds, forty full rounds
    std::vector<double> samples;
    for (int i = 0; i < kTicks; ++i) {
        const auto started = std::chrono::steady_clock::now();
        studio.game().pollFiles(100.0 + i * kTick);
        samples.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
    }
    double total = 0.0;
    for (const double ms : samples) total += ms;
    std::sort(samples.begin(), samples.end());
    const double average = total / kTicks;
    const double p95 = samples[static_cast<std::size_t>(kTicks * 0.95)];
    const double worst = samples.back();
    // The budget is 0.5 ms a frame on average (the game draws three frames to a tick, so a frame pays a third of the tick average); a tick that looks at
    // its twenty files costs a little more and the next ticks nothing.
    const std::string report = std::format("US-304 watcher cost on the shipped data ({} files, {} ticks of {} ms): average {:.3f} ms a tick, 95th percentile {:.3f} ms, worst {:.3f} ms (budget 0.5 ms a frame)\n",
                                           studio.game().watcher().files(), kTicks, static_cast<int>(kTick * 1000), average, p95, worst);
    MESSAGE(report);
#ifdef NDEBUG
    // GitHub's shared runners are a few times slower than the owner's PC and noisy (X-M11: 0.515 ms against 0.192 ms on the PC, same code): there the budget is three times as wide,
    // the strict one is judged on the owner's PC (the gate records it), like the first-frame limit (D-47).
#pragma warning(suppress : 4996)
    const double widen = std::getenv("GITHUB_ACTIONS") != nullptr ? 3.0 : 1.0;
    CHECK(average < 0.5 * widen);
    CHECK(p95 < 1.5 * widen); // the tick that looks at twenty files: a tenth of the 16 ms of a frame at most
#else
    CHECK(average < 20.0); // a Debug build with AddressSanitizer is many times slower; the Release run checks 0.5
#endif
    if (const std::string evidence = evidenceFolder(); !evidence.empty()) {
        fs::create_directories(evidence);
#ifdef NDEBUG
        writeText(fs::path(evidence) / "watch-cost-release.txt", report);
#else
        writeText(fs::path(evidence) / "watch-cost-debug.txt", report);
#endif
    }
}
