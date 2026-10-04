// US-152 and US-153: the context menu is made from the interaction files, and timed actions.
#include "camp.h"

using namespace camp_support;

TEST_CASE("US-152 The same actions are offered as before: a person, the fire and the knapping stone") {
    Camp camp("menu-same");
    // A clan member.
    const int person = camp.person();
    REQUIRE(person >= 0);
    const auto& figure = camp.odyssey.clanView().figures()[static_cast<std::size_t>(person)];
    const bool near = std::hypot(figure.x - camp.odyssey.hero().feetX(), figure.y - camp.odyssey.hero().feetY()) <= 64;
    auto menu = camp.menuAt(figure.x, figure.y - 8);
    CHECK(camp.odyssey.run().contextTitle() == camp.odyssey.clan()->people()[static_cast<std::size_t>(person)].name);
    REQUIRE(menu.size() >= 2);
    CHECK(menu[0] == (near ? "Talk" : "Talk | Too far away"));
    CHECK(menu[1] == (near ? "Give berries | You have no berries" : "Give berries | Too far away"));
    for (std::size_t i = 2; i < menu.size(); ++i) CHECK_MESSAGE(menu[i].rfind("Ask to teach you ", 0) == 0, menu[i]);

    // The clan's fire.
    const game::PixelPoint fire = camp.fire();
    menu = camp.menuAt(fire.x, fire.y - 12);
    CHECK(camp.odyssey.run().contextTitle() == "The clan's fire");
    REQUIRE(menu.size() == 4);
    CHECK(menu[0] == "Craft at the fire");
    CHECK(menu[1] == "Eat berries | You have no berries");
    CHECK(menu[2] == "Tend the fire");
    CHECK(menu[3] == "Tend the sacred fire | No sacred fire yet");

    // The knapping stone, 4 m east and 2 m south of the fire: too far for the hero standing at the fire.
    const game::PixelPoint stone = camp.odyssey.knappingStone();
    menu = camp.menuAt(stone.x, stone.y - 10);
    CHECK(camp.odyssey.run().contextTitle() == "Knapping stone");
    REQUIRE(menu.size() == 1);
    CHECK(menu[0] == "Craft | Too far away");

    // Nothing there: no menu.
    CHECK_FALSE(camp.odyssey.run().openContext(camp.odyssey, fire.x + 600, fire.y + 600));
}

TEST_CASE("US-152 Choosing an item does what it did before") {
    Camp camp("menu-acts");
    const game::PixelPoint fire = camp.fire();
    auto& life = *camp.odyssey.life();

    camp.menuAt(fire.x, fire.y - 12);
    camp.choose(0); // Craft at the fire opens the craft screen
    CHECK(camp.odyssey.run().screen() == game::Screen::Craft);
    camp.odyssey.run().close();

    life.gatherBerries();
    const int berries = life.count("berries");
    REQUIRE(berries >= 2);
    auto menu = camp.menuAt(fire.x, fire.y - 12);
    CHECK(menu[1] == "Eat berries"); // possible now
    camp.choose(1);
    CHECK(life.count("berries") == berries - 1);
    CHECK_FALSE(camp.odyssey.run().message().empty());

    camp.menuAt(fire.x, fire.y - 12);
    camp.odyssey.run().setMessage("");
    camp.choose(2); // Tend the fire
    CHECK_FALSE(camp.odyssey.run().message().empty());

    // A greyed-out item does nothing.
    camp.menuAt(fire.x, fire.y - 12);
    camp.odyssey.run().setMessage("untouched");
    camp.choose(3); // Tend the sacred fire: no sacred fire yet
    CHECK(camp.odyssey.run().message() == "untouched");
    CHECK(camp.odyssey.run().screen() == game::Screen::Context);

    // Talking to a clan member, and giving them a berry.
    const int person = camp.person();
    REQUIRE(person >= 0);
    const auto& figure = camp.odyssey.clanView().figures()[static_cast<std::size_t>(person)];
    camp.menuAt(figure.x, figure.y - 8);
    camp.odyssey.run().setMessage("");
    camp.choose(0); // Talk
    // Talk opens a conversation panel (a script, or small talk: US-161, US-163); the plain message is for people with neither.
    CHECK((camp.odyssey.run().screen() == game::Screen::Talk || !camp.odyssey.run().message().empty()));
    camp.odyssey.run().press(camp.odyssey, game::RunFlow::kClose);
    camp.menuAt(figure.x, figure.y - 8);
    camp.choose(1); // Give berries
    CHECK(life.count("berries") == berries - 2);
}

TEST_CASE("US-152 The sacred fire and its menu") {
    Camp camp("menu-sacred");
    auto& life = *camp.odyssey.life();
    const game::PixelPoint spot{camp.fire().x + 96, camp.fire().y};
    life.setSkillPoints(3, 30); // Fire-keeping level 3: the hero may found a sacred fire
    REQUIRE(life.foundFire("Hearth", spot.x / 32, spot.y / 32).ok);
    // At the clan's fire the sacred-fire item is possible now (it has no distance limit).
    auto menu = camp.menuAt(camp.fire().x, camp.fire().y - 12);
    REQUIRE(menu.size() == 4);
    CHECK(menu[3] == "Tend the sacred fire");
    // The sacred fire itself: 3 m from the hero, inside the 4 m reach, so both items are possible.
    menu = camp.menuAt((spot.x / 32) * 32 + 16, (spot.y / 32) * 32 + 16);
    CHECK(camp.odyssey.run().contextTitle() == "Sacred fire Hearth");
    REQUIRE(menu.size() == 2);
    CHECK(menu[0] == "Tend the fire");
    CHECK(menu[1] == "Hold a ritual");
}

TEST_CASE("US-152 The owner renames Tend the fire in tend-fire.json and F5 shows it") {
    const fs::path data = dataCopy("menu-rename");
    Camp camp("menu-rename", data);
    const game::PixelPoint fire = camp.fire();
    auto menu = camp.menuAt(fire.x, fire.y - 12);
    REQUIRE(menu.size() == 4);
    CHECK(menu[2] == "Tend the fire");

    const fs::path file = data / "interactions" / "tend-fire.json";
    std::string text = readText(file);
    const std::string from = "\"label\": \"Tend the fire\"";
    const std::size_t at = text.find(from);
    REQUIRE(at != std::string::npos);
    text.replace(at, from.size(), "\"label\": \"Feed the flames\"");
    writeText(file, text);
    camp.odyssey.update(reloadPressed());

    menu = camp.menuAt(fire.x, fire.y - 12);
    REQUIRE(menu.size() == 4);
    CHECK(menu[2] == "Feed the flames");
    CHECK(menu[0] == "Craft at the fire"); // everything else is as it was
}

TEST_CASE("US-152 A do naming a built-in action the game does not have is an error") {
    const fs::path data = dataCopy("menu-bad-do");
    writeText(data / "interactions" / "oops.json",
              "{ \"id\": \"oops\", \"label\": \"Oops\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"person\"] },\n \"effects\": [\"do talks\"] }");
    game::OdysseyGame odyssey(data, ODYSSEUS_DEMO_LEVEL);
    REQUIRE(odyssey.interactionReport().errors.size() == 1);
    CHECK(odyssey.interactionReport().errors[0].text().find("interactions/oops.json:2: do names \"talks\", which the game does not know (it knows: ") == 0);
    CHECK(odyssey.interactions().find("oops") == nullptr);
    CHECK(odyssey.interactions().find("talk") != nullptr); // the rest loaded
}

TEST_CASE("US-152 Every action the old menu had is now a file") {
    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR, ODYSSEUS_DEMO_LEVEL);
    REQUIRE(odyssey.interactionReport().errors.empty());
    REQUIRE(odyssey.interactionReport().warnings.empty());
    for (const char* id : {"talk", "give-berries", "ask-to-teach-hunter", "ask-to-teach-gatherer", "ask-to-teach-flint-knapper", "ask-to-teach-fire-keeper",
                           "ask-to-teach-shaman-healer", "craft-at-fire", "eat-berries", "tend-fire", "tend-sacred-fire-from-camp", "craft-at-stone",
                           "tend-sacred-fire", "hold-ritual", "barter", "gather", "knap", "pick-flint", "chop", "inspect"}) {
        CHECK_MESSAGE(odyssey.interactions().find(id) != nullptr, id);
    }
    // Every built-in action is used by some file (and every `do` names one the game has: checked at load).
    std::set<std::string> used;
    for (const auto& interaction : odyssey.interactions().all()) {
        for (const auto& effect : interaction.effects) {
            if (effect.verb == "do") used.insert(effect.args[0]->text);
        }
    }
    for (const std::string& name : game::builtInActionNames()) CHECK_MESSAGE(used.count(name) == 1, name);
}

// ---- US-153: timed actions and world state

TEST_CASE("US-153 Gather takes three seconds under a ring, then gives berries and hides the plant until it is ripe again") {
    Camp camp("timed-gather", {}, true);
    // Nobody greets in this test (US-162): a bubble over a head would add drawings and blur the count of the ring.
    for (const odysseus::sim::Person& person : camp.odyssey.clan()->people()) camp.odyssey.changeOpinion(person.id, camp.odyssey.life()->personId(), -100);
    REQUIRE(camp.odyssey.plants().size() == 1);
    const game::WorldPlant& plant = camp.odyssey.plants()[0];
    REQUIRE(plant.state == "ripe");
    const int before = camp.berries();

    CHECK(camp.odyssey.plantOffers(0)[0].interaction->id == "gather");
    camp.gather(); // what choosing Gather in the menu does
    REQUIRE(camp.odyssey.actions().running(0) != nullptr);
    CHECK(camp.berries() == before); // nothing yet

    // The ring is on screen while it fills.
    camp.renderer.clear();
    camp.odyssey.render(camp.renderer, 0.0);
    // Shadows (US-244) move with the people, so the draws are counted without them.
    const auto drawsWithoutShadows = [&camp] {
        return static_cast<std::size_t>(std::count_if(camp.renderer.draws().begin(), camp.renderer.draws().end(), [&camp](const auto& draw) { return !camp.odyssey.isShadowTexture(draw.texture); }));
    };
    const std::size_t withRing = drawsWithoutShadows();

    camp.play(30); // 1.5 s
    CHECK(camp.odyssey.actions().progress(0, camp.odyssey.actionClock()) == 50);
    CHECK(camp.berries() == before);
    camp.play(29);
    CHECK(camp.berries() == before); // one tick short
    camp.play(1);
    CHECK(camp.berries() == before + 2); // after 3 s
    CHECK(camp.odyssey.actions().running(0) == nullptr);
    CHECK(plant.state == "picked");
    CHECK_FALSE(plant.present()); // hidden
    CHECK(camp.odyssey.plantAtWorld(plant.feet.x, plant.feet.y - 8) == -1);

    camp.renderer.clear();
    camp.odyssey.render(camp.renderer, 0.0);
    CHECK(drawsWithoutShadows() < withRing); // the ring and the plant are gone

    // Greyed out while it waits, even though it cannot be clicked: asked directly.
    const auto offers = camp.odyssey.plantOffers(0);
    CHECK_FALSE(offers[0].enabled);
    CHECK(offers[0].reason == "Nothing to pick yet");

    camp.play(299);
    CHECK(plant.state == "picked");
    camp.play(1); // 15 s after the pick
    CHECK(plant.state == "ripe");
    CHECK(plant.present());
    CHECK(camp.odyssey.plantAtWorld(plant.feet.x, plant.feet.y - 8) == 0); // back in the same spot
}

TEST_CASE("US-153 Moving or attacking stops a timed action and it gives nothing") {
    for (const luna::engine::Intent stopper : {luna::engine::Intent::MoveRight, luna::engine::Intent::Attack}) {
        Camp camp("timed-interrupt", {}, true);
        const game::WorldPlant& plant = camp.odyssey.plants()[0];
        const int before = camp.berries();
        camp.gather();
        camp.play(20);
        REQUIRE(camp.odyssey.actions().running(0) != nullptr);
        luna::engine::Intents stop;
        stop.set(stopper, true, true);
        camp.play(1, stop);
        CHECK(camp.odyssey.actions().running(0) == nullptr);
        camp.play(200);
        CHECK(camp.berries() == before);
        CHECK(plant.state == "ripe");
        CHECK(plant.present());
    }
}

TEST_CASE("US-153 Standing still lets the action finish; opening a screen pauses it") {
    Camp camp("timed-pause", {}, true);
    const int before = camp.berries();
    camp.gather();
    camp.play(20);
    camp.odyssey.run().openMenu(); // the world waits while a screen is open
    camp.play(100);
    CHECK(camp.berries() == before);
    CHECK(camp.odyssey.actions().running(0) != nullptr);
    camp.odyssey.run().close();
    camp.play(40);
    CHECK(camp.berries() == before + 2);
}

TEST_CASE("US-153 A picked bush and a lit fire are still there after a save and a load") {
    const fs::path data = dataCopy("timed-save");
    Camp camp("timed-save", data, true);
    auto& life = *camp.odyssey.life();
    life.setSkillPoints(3, 30);
    const game::PixelPoint spot{camp.fire().x + 96, camp.fire().y};
    REQUIRE(life.foundFire("Hearth", spot.x / 32, spot.y / 32).ok);
    REQUIRE(life.fire().lit);

    const game::WorldPlant& plant = camp.odyssey.plants()[0];
    camp.gather();
    camp.play(60);
    REQUIRE(plant.state == "picked");
    camp.play(100); // 5 s into the ripening: 10 s are left
    REQUIRE(camp.odyssey.autosave());
    const int berries = camp.berries();

    // A new game over the same folder, as after closing and starting the game again.
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(data, Camp::makeLevel(data, "timed-save", true));
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    REQUIRE(again.plants().size() == 1);
    CHECK(again.plants()[0].state == "picked"); // still picked
    CHECK_FALSE(again.plants()[0].present());
    REQUIRE(again.life() != nullptr);
    CHECK(again.life()->fire().founded); // the fire is still lit
    CHECK(again.life()->fire().lit);
    CHECK(again.life()->count("berries") == berries);

    for (int i = 0; i < 199; ++i) again.update({});
    CHECK(again.plants()[0].state == "picked");
    again.update({}); // 10 s after loading: the 15 s are over
    CHECK(again.plants()[0].state == "ripe");
}

TEST_CASE("US-153 Things that never changed save nothing, and a damaged things file is reported") {
    const fs::path data = dataCopy("timed-clean");
    Camp camp("timed-clean", data, true);
    REQUIRE(camp.odyssey.autosave());
    const std::string text = readText(data.parent_path() / "saves" / "things.json");
    CHECK(text.find("\"plants\": {}") != std::string::npos); // a ripe plant is the default: not written
    CHECK(text.find("\"pending\": []") != std::string::npos);

    writeText(data.parent_path() / "saves" / "things.json", "{ not json");
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(data, Camp::makeLevel(data, "timed-clean", true));
    again.start(renderer);
    CHECK(again.loadAutosave()); // the clan and the hero still load
    CHECK(again.message().find("things.json could not be read") != std::string::npos);
}

// ---- US-155: the world objects of the first Age

TEST_CASE("US-155 objects.json has the seven objects with tags and states") {
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    std::vector<std::string> objects;
    for (const game::PlantDef& plant : catalogs.plants) {
        if (plant.object && !plant.celestial) objects.push_back(plant.name);
    }
    CHECK(objects == std::vector<std::string>{"fire pit", "knapping stone", "food store", "shelter", "flint nodule", "water source", "sleeping furs"});
    const auto tagged = [&](const std::string& name, const std::string& tag) {
        const game::PlantDef* def = catalogs.plant(name);
        REQUIRE(def != nullptr);
        return std::find(def->tags.begin(), def->tags.end(), tag) != def->tags.end();
    };
    CHECK(tagged("fire pit", "fire-pit"));
    CHECK(tagged("knapping stone", "knapping-stone"));
    CHECK(tagged("food store", "store"));
    CHECK(tagged("shelter", "shelter"));
    CHECK(tagged("flint nodule", "flint-nodule"));
    CHECK(tagged("water source", "water"));
    CHECK(tagged("sleeping furs", "bedding"));
    CHECK(catalogs.plant("fire pit")->states == std::vector<std::string>{"cold", "burning"});
    CHECK(catalogs.plant("food store")->states == std::vector<std::string>{"empty", "stocked"});
    CHECK(catalogs.plant("shelter")->states.empty());
    // The plants are untouched: 153 of them, none an object.
    CHECK(std::count_if(catalogs.plants.begin(), catalogs.plants.end(), [](const game::PlantDef& p) { return !p.object; }) == 153);
    // Objects are in the Editor's list of what may be placed.
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    CHECK(definitions.objects.size() == 9); // the seven, then the two bodies that may be placed (US-248)
    CHECK(definitions.hasPlant("fire pit"));
}

TEST_CASE("US-155 A new object kind added to objects.json shows up with its tags, and mistakes are named") {
    const fs::path data = dataCopy("objects-data");
    std::string text = readText(data / "objects.json");
    const std::string marker = "\"objects\": [";
    text.insert(text.find(marker) + marker.size(), "\n    { \"name\": \"stone cairn\", \"frame\": \"cairn\", \"blocks\": false, \"inspect\": \"A heap of stones.\", \"tags\": [\"object\", \"landmark\"] },");
    writeText(data / "objects.json", text);
    const game::Catalogs catalogs = game::loadCatalogs(data);
    const game::PlantDef* cairn = catalogs.plant("stone cairn");
    REQUIRE(cairn != nullptr);
    CHECK(cairn->object);
    CHECK(cairn->tags == std::vector<std::string>{"object", "landmark"});
    CHECK(catalogs.knownTags().count("landmark") == 1);
    CHECK(game::loadDefinitions(data).objects.size() == 10);
    // An object with no tags written is still tagged "object", so Inspect works on it.
    text = readText(data / "objects.json");
    text.insert(text.find(marker) + marker.size(), "\n    { \"name\": \"plain\", \"frame\": \"x\", \"blocks\": false, \"inspect\": \"Plain.\" },");
    writeText(data / "objects.json", text);
    CHECK(game::loadCatalogs(data).plant("plain")->tags == std::vector<std::string>{"object"});

    // Mistakes name the file and the field.
    const auto problem = [&](const std::string& entry) {
        const fs::path bad = dataCopy("objects-bad");
        std::string t = readText(bad / "objects.json");
        t.insert(t.find(marker) + marker.size(), "\n" + entry + ",");
        writeText(bad / "objects.json", t);
        try {
            game::loadCatalogs(bad);
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    CHECK(problem("{ \"name\": \"x\", \"blocks\": false, \"inspect\": \"i\" }").find("objects.json: objects[0].frame") != std::string::npos);
    CHECK(problem("{ \"name\": \"wheat\", \"frame\": \"x\", \"blocks\": false, \"inspect\": \"i\" }").find("\"wheat\" is already a plant") != std::string::npos);
    CHECK(problem("{ \"name\": \"y\", \"frame\": \"x\", \"blocks\": false, \"inspect\": \"i\", \"tags\": [\"a b\"] }").find("objects[0].tags[0]") != std::string::npos);
}

TEST_CASE("US-155 The Editor places a fire pit and it is saved in the level") {
    const fs::path data = dataCopy("objects-editor");
    luna::engine::RecordingRenderer renderer;
    const fs::path file = Camp::makeLevel(data, "objects-editor");
    game::OdysseyGame odyssey(data, file);
    odyssey.start(renderer);
    luna::engine::Intents toEditor;
    toEditor.set(luna::engine::Intent::ModeEditor, true, true);
    odyssey.update(toEditor);
    game::Editor& editor = odyssey.editor();

    const game::Definitions definitions = game::loadDefinitions(data);
    const int firePit = static_cast<int>(definitions.plants.size()); // the objects follow the plants in the palette: the fire pit is the first
    editor.setTool(game::EditorTool::Plant);
    editor.setPlant(firePit);
    const auto view = editor.camera().view();
    const auto mouse = [&](bool press, bool hold, bool release) {
        luna::engine::Intents intents;
        luna::engine::Pointer pointer;
        pointer.x = 1100 - view.x;
        pointer.y = 1090 - view.y;
        const auto left = static_cast<std::size_t>(luna::engine::PointerButton::Left);
        pointer.pressed[left] = press;
        pointer.held[left] = hold;
        pointer.released[left] = release;
        intents.setPointer(pointer);
        return intents;
    };
    odyssey.update(mouse(true, true, false)); // a click: down, then up
    odyssey.update(mouse(false, false, true));    REQUIRE(editor.level().plants.size() == 1);
    CHECK(editor.level().plants[0].kind == "fire pit");
    REQUIRE(editor.save());
    CHECK(game::loadLevel(file, definitions).level.plants == editor.level().plants); // saved, and it loads again
    CHECK(editor.undo());
    CHECK(editor.level().plants.empty());

    // The palette: the plants fill their pages, the objects have the last page to themselves.
    editor.setTool(game::EditorTool::Plant);
    odyssey.update({});
    CHECK(editor.plantPage() == 0);
}

TEST_CASE("US-155 Light a fire in a fire pit with a fire drill, and it warms people nearby") {
    Camp camp("objects-fire", {}, false, "fire pit");
    REQUIRE(camp.odyssey.plants().size() == 1);
    const game::WorldPlant& pit = camp.odyssey.plants()[0];
    CHECK(pit.state == "cold");
    CHECK(pit.present()); // an object is always there: its states never hide it
    CHECK(game::plantSubject(camp.odyssey, 0).info.tags == std::vector<std::string>{"object", "fire", "fire-pit"});

    // No drill: greyed out, with the reason.
    auto offers = camp.odyssey.plantOffers(0);
    REQUIRE(offers.size() == 2);
    CHECK(offers[0].interaction->id == "light-fire");
    CHECK_FALSE(offers[0].enabled);
    CHECK(offers[0].reason == "You need a fire drill");
    CHECK(offers[1].interaction->id == "inspect-object");

    // The hero gets a fire drill and lights the fire.
    camp.odyssey.life()->give("fire-drill", 1);
    offers = camp.odyssey.plantOffers(0);
    CHECK(offers[0].enabled);
    REQUIRE(game::startInteraction(camp.odyssey, "light-fire", game::plantSubject(camp.odyssey, 0)));
    camp.play(59);
    CHECK(pit.state == "cold"); // three seconds to light it
    camp.play(1);
    CHECK(pit.state == "burning");
    CHECK(camp.odyssey.run().message().rfind("The fire warms ", 0) == 0); // and says how many people it warmed
    CHECK(camp.odyssey.run().message() != "The fire warms 0 people.");
    CHECK(camp.odyssey.actions().pending().size() == 3); // two more warmings, and the fire going out

    // While it burns, lighting it again is not offered as possible.
    offers = camp.odyssey.plantOffers(0);
    CHECK_FALSE(offers[0].enabled);
    CHECK(offers[0].reason == "It is already burning");
}

TEST_CASE("US-155 The other objects have their actions") {
    {   // The food store takes two berries in and gives them back.
        Camp camp("objects-store", {}, false, "food store");
        auto& life = *camp.odyssey.life();
        life.gatherBerries();
        const int berries = life.count("berries");
        auto offers = camp.odyssey.plantOffers(0);
        REQUIRE(offers.size() == 3); // put in, take out, inspect
        CHECK(offers[0].interaction->id == "put-in-store");
        CHECK(offers[0].enabled);
        CHECK_FALSE(offers[1].enabled);
        CHECK(offers[1].reason == "The store is empty");
        REQUIRE(game::startInteraction(camp.odyssey, "put-in-store", game::plantSubject(camp.odyssey, 0)));
        camp.play(40);
        CHECK(life.count("berries") == berries - 2);
        CHECK(camp.odyssey.plants()[0].state == "stocked");
        REQUIRE(game::startInteraction(camp.odyssey, "take-from-store", game::plantSubject(camp.odyssey, 0)));
        camp.play(40);
        CHECK(life.count("berries") == berries);
        CHECK(camp.odyssey.plants()[0].state == "empty");
    }
    {   // The flint nodule gives flint, then needs two minutes.
        Camp camp("objects-flint", {}, false, "flint nodule");
        auto& life = *camp.odyssey.life();
        const int flint = life.count("flint");
        REQUIRE(game::startInteraction(camp.odyssey, "pick-nodule-flakes", game::plantSubject(camp.odyssey, 0)));
        camp.play(40);
        CHECK(life.count("flint") == flint + 1);
        CHECK(camp.odyssey.plants()[0].state == "chipped");
        const auto offers = camp.odyssey.plantOffers(0);
        CHECK_FALSE(offers[0].enabled);
        CHECK(offers[0].reason == "Nothing left to chip");
    }
    {   // Sleeping on the furs restores energy; the shelter and the water only have what they say.
        Camp camp("objects-sleep", {}, false, "sleeping furs");
        REQUIRE(game::startInteraction(camp.odyssey, "sleep-on-furs", game::plantSubject(camp.odyssey, 0)));
        camp.play(160);
        CHECK(camp.odyssey.run().message() == "You feel better: warmth +10.");
    }
    {
        Camp camp("objects-water", {}, false, "water source");
        REQUIRE(game::startInteraction(camp.odyssey, "drink", game::plantSubject(camp.odyssey, 0)));
        camp.play(40);
        CHECK(camp.odyssey.run().message() == "The water is cold and clear.");
    }
    {   // The knapping stone object opens the same crafting as the camp's stone.
        Camp camp("objects-stone", {}, false, "knapping stone");
        const auto offers = camp.odyssey.plantOffers(0);
        REQUIRE(offers.size() == 2);
        CHECK(offers[0].interaction->id == "craft-at-stone");
        CHECK(offers[0].enabled);
        REQUIRE(game::startInteraction(camp.odyssey, "craft-at-stone", game::plantSubject(camp.odyssey, 0)));
        CHECK(camp.odyssey.run().screen() == game::Screen::Craft);
    }
}

// ---- US-233: every screen is laid out from the interface size

TEST_CASE("US-233 Screens fit the interface at both UI scales") {
    for (const int scale : {1, 2}) {
        Camp camp("layout-" + std::to_string(scale), {}, true);
        camp.odyssey.setViewScales(2, scale);
        const int width = camp.odyssey.uiWidth();
        const int height = camp.odyssey.uiHeight();
        CHECK(width == 960 / scale);
        auto& run = camp.odyssey.run();
        for (const int tab : {300, 301, 302, 303}) { // Bag, Skills, Dominion, Settings
            run.openMenu();
            camp.play(1);
            if (tab != 300) {
                REQUIRE(run.press(camp.odyssey, tab));
                camp.play(1);
            }
            const auto& widgets = run.widgets();
            REQUIRE_FALSE(widgets.empty());
            for (std::size_t a = 0; a < widgets.size(); ++a) {
                const auto& r = widgets[a].area;
                INFO("scale " << scale << " tab " << tab << " widget " << widgets[a].label);
                CHECK(r.x >= 0);
                CHECK(r.y >= 0);
                CHECK(r.x + r.width <= width);
                CHECK(r.y + r.height <= height);
                for (std::size_t b = a + 1; b < widgets.size(); ++b) { // nothing overlaps
                    const auto& s = widgets[b].area;
                    const bool apart = r.x + r.width <= s.x || s.x + s.width <= r.x || r.y + r.height <= s.y || s.y + s.height <= r.y;
                    CHECK_MESSAGE(apart, widgets[a].label << " overlaps " << widgets[b].label);
                }
            }
            run.close();
        }
    }
}
