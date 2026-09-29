#include <doctest/doctest.h>

#include <cstdlib>
#include <string>

namespace {

// CMake runs each real compiler probe and checks its diagnostic, not just its exit code.
void checkProbe(const char* name, bool allowed, const char* header = "") {
    const std::string command =
        "cmake -DBUILD_DIR=\"" ODYSSEUS_BUILD_DIR "\" -DCONFIG=" ODYSSEUS_TEST_CONFIGURATION
        " -DPROBE=" + std::string(name) + " -DEXPECT_SUCCESS=" + (allowed ? "ON" : "OFF") +
        " -DEXPECTED_HEADER=\"" + header + "\" -P \"" ODYSSEUS_PROBE_SCRIPT "\"";
    INFO("Compiler probe: ", std::string(name));
    CHECK(std::system(command.c_str()) == 0);
}

} // namespace

TEST_CASE("US-003 Allowed use") {
    checkProbe("us003_game_engine", true);
    checkProbe("us003_game_sim", true);
    checkProbe("us003_sim_core", true);
    checkProbe("us003_engine_platform", true);
    checkProbe("us003_platform_core", true);
}

TEST_CASE("US-003 Forbidden use") {
    checkProbe("us003_sim_engine", false, "luna/engine/layer.h");
    checkProbe("us003_sim_sdl", false, "SDL3/SDL.h");
    checkProbe("us003_sim_engine_absolute", false, "Engine|ENGINE|engine");
    checkProbe("us003_sim_engine_relative", false, "Engine|ENGINE|engine");
}

TEST_CASE("US-003 Luna stays game-agnostic") {
    checkProbe("us003_engine_sim", false, "sim/layer.h");
    checkProbe("us003_engine_game", false, "game/layer.h");
    checkProbe("us003_platform_sim", false, "sim/layer.h");
    checkProbe("us003_platform_game", false, "game/layer.h");
    checkProbe("us003_engine_sim_relative", false, "Simulation|SIM|sim");
}

TEST_CASE("US-003 Guard completeness") {
    const std::string command =
        "cmake -DSOURCE_DIR=\"" ODYSSEUS_SOURCE_DIR "\" -DTEST_DIR=\"" ODYSSEUS_BUILD_DIR
        "/architecture-validator\" -P \"" ODYSSEUS_VALIDATOR_SCRIPT "\"";
    CHECK(std::system(command.c_str()) == 0);
}

TEST_CASE("US-025 Physics layer rules") {
    // Allowed: Physics uses Core; Engine, Simulation and Game use Physics.
    checkProbe("us025_physics_core", true);
    checkProbe("us025_engine_physics", true);
    checkProbe("us025_sim_physics", true);
    checkProbe("us025_game_physics", true);
    // Forbidden: Physics never reaches up or sideways, and never sees SDL3.
    checkProbe("us025_physics_platform", false, "luna/platform/layer.h");
    checkProbe("us025_physics_engine", false, "luna/engine/layer.h");
    checkProbe("us025_physics_sim", false, "sim/layer.h");
    checkProbe("us025_physics_game", false, "game/layer.h");
    checkProbe("us025_physics_sdl", false, "SDL3/SDL.h");
    checkProbe("us025_physics_engine_absolute", false, "Engine|ENGINE|engine");
    // Platform sits beside Physics, not above it.
    checkProbe("us025_platform_physics", false, "Physics|PHYSICS|physics");
}
