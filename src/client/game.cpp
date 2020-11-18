#include <SDL2/SDL_timer.h>

#include <client/game.hpp>
#include <client/graphical/animator.hpp>
#include <client/graphical/gfx_debug_drawer.hpp>
//#include <client/graphical/gl_renderer.hpp>
#include <client/graphical/light.hpp>
#include <client/input/input_service.hpp>
#include <common/logger.hpp>
#include <common/logic/colony.hpp>
#include <common/logic/game_event.hpp>
#include <common/logic/logic_service.hpp>
#include <exception>

#include "client/graphical/exceptions.hpp"
#include "common/logic/PathFinder.hpp"

using namespace familyline;
using namespace familyline::logic;
using namespace familyline::graphics;
using namespace familyline::graphics::gui;

/**
 * Initialize a map
 *
 * Be aware that this will not reset anything related to
 * objects, so you should call `initObjects`, or you will
 * be screwed
 */
logic::Terrain& Game::initMap(std::string_view path)
{
    auto& log = LoggerService::getLogger();

    if (!terrFile_->open(path)) {
        throw new logic_exception{
            fmt::format("Could not open terrain " ASSET_FILE_DIR " terrain_test.flte")};
    }

    terrain_ = std::move(std::make_unique<Terrain>(*terrFile_.get()));

    /// Initial position of the camera might be part of the map file
    /// in the future, so we start it here too.
    int winW, winH;
    window_->getSize(winW, winH);

    glm::vec3 directionOffset = glm::vec3(6.0f, 36.0f, 6.0f);
    glm::vec3 position        = glm::vec3(0, 0, 0);
    float aspectRatio         = float(winW) / float(winH);
    camera_ = std::make_unique<Camera>(position + directionOffset, aspectRatio, position);

    terr_rend_ = std::make_unique<TerrainRenderer>(*terrain_.get(), *camera_.get());
    terr_rend_->buildVertexData();
    terr_rend_->buildTextures();

    log->write("game", LogType::Info, "map '%s' loaded", path.data());

    return *terrain_.get();
}

/**
 * Pass the player manager to the game
 *
 * Please add all starting clients to the player manager before starting the
 * game.
 *
 * When we support spectators (of course we will), the add/removal of the
 * spectators need to happen outside
 */
void Game::initPlayers(
    std::unique_ptr<logic::PlayerManager> pm, std::unique_ptr<logic::ColonyManager> cm,
    decltype(colonies_) player_colony_map, int human_id)
{
    auto& log = LoggerService::getLogger();
    log->write("game", LogType::Info, "configuring player manager");

    cm_.swap(cm);
    pm_.swap(pm);
    pm_->render_add_callback = [&](std::shared_ptr<GameObject> o) {
        if (objrend_) objrend_->add(o);
    };
    colonies_ = player_colony_map;

    pm_->colony_add_callback = [&](std::shared_ptr<GameObject> o, unsigned player_id) {
        auto& col = o->getColonyComponent();
        if (col.has_value()) {
            col->owner = std::make_optional(colonies_.at(player_id));
            log->write(
                "game", LogType::Info, "set object id %d (%s) to city %s", o->getID(),
                o->getName().c_str(), col->owner.value().get().getName().data());
        }
    };

    if (human_id != -1) {
        HumanPlayer* hp = (HumanPlayer*)*pm_->get(human_id);
        hp->setCamera(camera_.get());
    }

    log->write("game", LogType::Info, "player manager configured");
}

void Game::initObjects()
{
    auto& log = LoggerService::getLogger();
    log->write("game", LogType::Info, "configuring game objects");

    rndr_ = window_->createRenderer();

    // TODO: move this outside?
    AssetFile f;
    f.loadFile("assets.yml");
    am->loadFile(f);

    om_  = std::make_unique<ObjectManager>();
    olm_ = std::make_unique<ObjectLifecycleManager>(*om_.get());

    auto& of = LogicService::getObjectFactory();

    /* Adds the objects to the factory */
    of->addObject(new WatchTower);
    of->addObject(new Tent);

    pathf_ = std::make_unique<PathFinder>(om_.get());

    {
        auto [w, h] = terrain_->getSize();
        pathf_->InitPathmap(w, h);
        pathf_->UpdatePathmap(w, h);
    }

    pm_->olm = olm_.get();
    pm_->pf  = pathf_.get();

    log->write("game", LogType::Info, "game objects configured");
}

auto sunlight = std::make_unique<Light>(
    SunLightType{glm::vec3(-0.2, -1.0, -0.2)}, 2.5f, glm::vec3(0.1, 0.2, 0.8), "sunlight");

auto pointlight = std::make_unique<Light>(
    PointLightType{glm::vec3(30.0, 10.0, 30.0)}, 5.8f, glm::vec3(0, 0.8, 0.2), "greenishlight");

auto pointlight2 = std::make_unique<Light>(
    PointLightType{glm::vec3(50.0, 10.0, 10.0)}, 9.8f, glm::vec3(0.8, 0.2, 0.0), "redishlight");

void Game::initLoopData(int human_id)
{        
    auto& log = LoggerService::getLogger();
    log->write("game", LogType::Info, "initializing other data needed by the game");

    LogicService::initDebugDrawer(new GFXDebugDrawer(*rndr_, *terrain_.get()));

    //// Initialize some graphical data

    gctx    = {};
    gctx.om = om_.get();
    // gam.AddListener(new GameActionListenerImpl());

    scenernd_ = std::make_unique<SceneManager>(*rndr_, *camera_.get());
    scenernd_->add(std::make_shared<SceneObject<Light>>(*sunlight.get()));
    scenernd_->add(std::make_shared<SceneObject<Light>>(*pointlight.get()));
    scenernd_->add(std::make_shared<SceneObject<Light>>(*pointlight2.get()));

    objrend_ = std::make_unique<ObjectRenderer>(*terrain_.get(), *scenernd_.get());

    ///// Initialize input

    ip_ = std::make_unique<input::InputPicker>(
        terrain_.get(), window_, scenernd_.get(), camera_.get(), om_.get());

    if (human_id != -1) {
        HumanPlayer* hp = (HumanPlayer*)*pm_->get(human_id);
        hp->SetPicker(ip_.get());
    }

    ObjectPathManager::getInstance()->SetTerrain(terrain_.get());

    /// add the labels
    widgets.lblBuilding   = new Label(0.05 * 640, 0.1 * 480, "!!!");
    widgets.lblFPS        = new Label(0.05 * 640, 0.9 * 480, "0 fps, 0 ms/frame");
    widgets.lblRange      = new Label(0.05 * 640, 0.13 * 480, "--");
    widgets.lblSelected   = new Label(0.05 * 640, 0.17 * 480, "---");
    widgets.lblTerrainPos = new Label(0.05 * 640, 0.21 * 480, "---");
    widgets.lblKeys       = new Label(
        0.05 * 640, 0.05 * 480,
        "Press C to build Tent, E to build WatchTower, B to draw bounding boxes");

    widgets.lblBuilding->modifyAppearance([](ControlAppearance& ca) {
        ca.foreground = {1, 1, 1, 1};
        ca.background = {0, 0, 0, 0.4};
    });

    widgets.lblFPS->modifyAppearance([](ControlAppearance& ca) {
        ca.foreground = {1, 1, 1, 1};
        ca.background = {0, 0, 0, 0.4};
    });

    widgets.lblRange->modifyAppearance([](ControlAppearance& ca) {
        ca.foreground = {1, 1, 1, 1};
        ca.background = {0, 0, 0, 0.4};
    });

    widgets.lblSelected->modifyAppearance([](ControlAppearance& ca) {
        ca.foreground = {1, 1, 1, 1};
        ca.background = {0, 0, 0, 0.4};
    });

    widgets.lblKeys->modifyAppearance([](ControlAppearance& ca) {
        ca.foreground = {0.9, 0.8, 1, 1};
        ca.background = {0, 0, 0, 0.4};
    });

    widgets.lblTerrainPos->modifyAppearance([](ControlAppearance& ca) {
        ca.background = {0, 0, 0, 0.4};
    });

    GUIWindow& gw = gui_->getDebugWindow();
    gw.add(5, 5, ControlPositioning::Pixel, std::unique_ptr<Control>(widgets.lblFPS));
    gw.add(
        5, 35, ControlPositioning::Pixel,
        std::unique_ptr<Control>((Control*)widgets.lblTerrainPos));
    gw.add(
        5, 65, ControlPositioning::Pixel, std::unique_ptr<Control>((Control*)widgets.lblBuilding));
    gw.add(5, 95, ControlPositioning::Pixel, std::unique_ptr<Control>((Control*)widgets.lblRange));
    gw.add(
        5, 125, ControlPositioning::Pixel, std::unique_ptr<Control>((Control*)widgets.lblSelected));
    gw.add(5, 155, ControlPositioning::Pixel, std::unique_ptr<Control>((Control*)widgets.lblKeys));

    started_ = true;

    ticks_ = std::chrono::high_resolution_clock::now();

    log->write("game", LogType::Info, "game class ready");

}

Game::~Game()
{
    if (started_) {
        GUIWindow& gw = gui_->getDebugWindow();
        gw.remove(widgets.lblFPS);
        gw.remove(widgets.lblTerrainPos);
        gw.remove(widgets.lblBuilding);
        gw.remove(widgets.lblRange);
        gw.remove(widgets.lblSelected);
        gw.remove(widgets.lblKeys);
    }
}

bool Game::runLoop()
{
    rendertime_ = std::chrono::high_resolution_clock::now();
    bool player = true;

    /* Runs the input code at fixed steps, like the logic one below */
    while (inputTime >= INPUT_DELTA) {
        player = this->runInput();
        if (!player) {
            return false;
        }

        inputTime -= INPUT_DELTA;
    } 

    /* Run the logic code in steps of fixed blocks
     * This is called fixed timestep, and will ensure game consistency
     * on multiplayer games
     */
    int li = 0;
    while (logicTime >= LOGIC_DELTA) {
        this->runLogic();
        logicTime -= LOGIC_DELTA;
        li++;
        gctx.tick++;
    }

    if (frame_ > 1) limax = std::max(li, limax);

    this->showDebugInfo();
    this->runGraphical(double(delta.count()));
    Timer::getInstance()->RunTimers(delta.count());

    frame_++;

    ////////////////////////

    auto elapsed                = std::chrono::high_resolution_clock::now();
    delta                       = elapsed - ticks_;
    decltype(delta) renderdelta = std::chrono::high_resolution_clock::now() - rendertime_;

    if (frame_ % 15 == 0) {
        pms = delta.count() * 1.0;
    }

    ticks_ = elapsed;

    char sfps[128];
    sprintf(sfps, "%.3f fps, %.3f ms/frame", float(1000 / pms), float(pms));
    widgets.lblFPS->setText(sfps);
    //      gr->DebugWrite(0, 420, "%.2f ms, %.2f fps", pms, 1000 / pms);

#define FPS_LOCK 120.0

    // Locked in ~120 fps
    if (renderdelta.count() < (1000.0 / FPS_LOCK)) {
        auto sleepdelta = (1000.0 / FPS_LOCK) - 5;
        SDL_Delay(unsigned(sleepdelta));
    }

    // Make the mininum and maximum frame calculation more fair
    // because usually the first frame is when we load things, and
    // its the slowest.

    if (delta.count() > 0) {
        if (delta.count() < mindelta && frame_ > 2) mindelta = delta.count();

        if (delta.count() > maxdelta && frame_ > 2) maxdelta = delta.count();

        sumfps += (1000 / delta.count());
    }

    logicTime += delta.count();
    inputTime += delta.count();

    return true;
}

bool Game::runInput()
{
    /* Input processing  */

    input::InputService::getInputManager()->processEvents();

    ip_->UpdateIntersectedObject();
    ip_->UpdateTerrainProjectedPosition();

    gctx.elapsed_seconds = INPUT_DELTA / 1000.0;

    pm_->generateInput();
    gui_->update();

    return !pm_->exitRequested();
}

void Game::runLogic()
{
    pm_->run(gctx);
    olm_->update();

    LogicService::getObjectListener()->updateObjects();

    /* Logic & graphical processing */
    gam.ProcessListeners();
    // terr_rend->Update();
    om_->update();

    LogicService::getActionQueue()->processEvents();

    bool objupdate = objrend_->willUpdate();
    if (objupdate) {
        objrend_->update();
        auto [w, h] = terrain_->getSize();
        pathf_->UpdatePathmap(w, h);
    }

    LogicService::getAttackManager()->processAttacks(*olm_.get());
    ObjectPathManager::getInstance()->UpdatePaths(LOGIC_DELTA);

    LogicService::getDebugDrawer()->update();
}

void Game::runGraphical(double framems)
{
    /* Rendering */

    fb3D_->startDraw();
    terr_rend_->render(*rndr_);

    //  rndr->SetBoundingBox(hp->renderBBs);

    scenernd_->update(framems);

    //    rndr->UpdateObjects();

    //    rndr->UpdateFrames();
    rndr_->render(camera_.get());

    fb3D_->endDraw();

    fbGUI_->startDraw();
    gui_->render(0, 0);
    //    gr->renderToScreen();
    fbGUI_->endDraw();

    window_->update();
}

/* Show on-screen debug info
 * (aka the words in monospaced font you see in-game)
 */
void Game::showDebugInfo()
{
    const int human_id_ = 1;
    pm_->iterate([&](Player* p) {
        if (p->getCode() == human_id_) {
            this->showHumanPlayerInfo(p);
        }
    });
}

void Game::showHumanPlayerInfo(logic::Player* hp)
{
    auto selections = hp->getSelections();
    std::shared_ptr<GameObject> selected =
        selections.size() == 1 ? selections[0].lock() : std::shared_ptr<GameObject>();

    if (BuildQueue::GetInstance()->getNext()) {
        char s[256];
        sprintf(
            s, "Click to build %s",
            BuildQueue::GetInstance()->getNext().value()->getName().c_str());
        widgets.lblBuilding->setText(s);
    } else {
        widgets.lblBuilding->setText("");
    }

    auto locc = ip_->GetIntersectedObject().lock();
    if (locc && selected) {
        auto alocc = locc.get();

        bool attackable = selected->getAttackComponent().has_value();

        if (alocc && attackable && selected && alocc->getMaxHealth()) {
            auto inRange = selected->getAttackComponent()->isInAttackRange(
                alocc->getAttackComponent().value());

            widgets.lblRange->setText(inRange ? "In range" : "Not in range");
        }
    }

    widgets.lblSelected->setText("");
    if (selected) {
        char s[150];
        auto& acomp = selected->getAttackComponent();

        if (acomp) {
            sprintf(
                s, "Selected object: '%s' (%4f/%4d)", selected->getName().c_str(),
                selected->getHealth(), selected->getMaxHealth());
        } else {
            sprintf(s, "Selected object: '%s'", selected->getName().c_str());
        }

        widgets.lblSelected->setText(s);
    }

    glm::vec3 p = ip_->GetTerrainProjectedPosition();
    glm::vec2 q = ip_->GetGameProjectedPosition();

    char texs[256];
    sprintf(
        texs,
        "Terrain pos: "
        "(ogl: %.3f,%.3f,%.3f | Game: %.2f, %.2f), rotation %.1f",
        p.x, p.y, p.z, q.x, q.y, camera_->GetRotation() * 180 / M_PI);
    widgets.lblTerrainPos->setText(texs);
}

/// Return maximum, minimum and average fps
std::tuple<double, double, double> Game::getStatisticInfo()
{
    // less delta, more fps

    if (frame_ > 0 && started_)
        return std::make_tuple(1000 / mindelta, 1000 / maxdelta, sumfps / frame_);
    else
        return std::make_tuple(0.0, 0.0, 0.0);
}