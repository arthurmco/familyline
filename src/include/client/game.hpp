#pragma once

/**
 * Familyline game loop class
 */

#include <memory>

//#include "logic/ObjectRenderer.hpp"
#include <client/preview_renderer.hpp>
#include <client/graphical/GraphicalPlotInterface.hpp>
#include <client/graphical/TextureOpener.hpp>
#include <client/graphical/asset_file.hpp>
#include <client/graphical/asset_manager.hpp>
#include <client/graphical/camera.hpp>
#include <client/graphical/framebuffer.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/gui/gui_button.hpp>
#include <client/graphical/gui/gui_label.hpp>
#include <client/graphical/gui/gui_manager.hpp>
#include <client/graphical/material_manager.hpp>
#include <client/graphical/materialopener/MTLOpener.hpp>
#include <client/graphical/meshopener/OBJOpener.hpp>
#include <client/graphical/object_renderer.hpp>
#include <client/graphical/renderer.hpp>
#include <client/graphical/scene_manager.hpp>
#include <client/graphical/terrain_renderer.hpp>
#include <client/graphical/texture_manager.hpp>
#include <client/graphical/window.hpp>
#include <common/logic/BuildQueue.hpp>
#include <common/logic/GameActionManager.hpp>
#include <common/logic/object_path_manager.hpp>
#include <common/logic/pathfinder.hpp>
#include <common/logic/colony_manager.hpp>
#include <common/logic/debug_drawer.hpp>
#include <common/logic/lifecycle_manager.hpp>
#include <common/logic/object_factory.hpp>
#include <common/logic/player_manager.hpp>
#include <common/logic/terrain_file.hpp>
#include <common/logic/input_recorder.hpp>
#include <common/logic/input_reproducer.hpp>
//#include "graphical/gui/ImageControl.hpp"

//#include <client/input/InputPicker.hpp>
#include <client/HumanPlayer.hpp>
#include <client/Timer.hpp>
#include <client/input/input_manager.hpp>
#include <common/objects/Tent.hpp>
#include <common/objects/WatchTower.hpp>

namespace familyline
{
struct GFXGameInit {
    graphics::Window* window;
    graphics::Framebuffer* fb3D;
    graphics::Framebuffer* fbGUI;
    graphics::gui::GUIManager* gui;
};

class Game
{
public:
    Game(GFXGameInit& gi)
        : terrFile_(std::make_unique<logic::TerrainFile>()),
          window_(gi.window),
          fb3D_(gi.fb3D),
          fbGUI_(gi.fbGUI),
          gui_(gi.gui),
          camera_(std::make_unique<graphics::Camera>(
                      glm::vec3(6.0, 36.0, 6.0), 16.0 / 9.0f, glm::vec3(0))),
          am(graphics::GFXService::getAssetManager())
    {
    }

    ~Game();

    /**
     * Initialize a map
     *
     * Be aware that this will not reset anything related to
     * objects, so you should call `initObjects`, or you will
     * be screwed
     */
    logic::Terrain& initMap(std::string_view path);

    /**
     * Pass the player manager to the game
     *
     * Please add all starting clients to the player manager before starting the
     * game.
     *
     * When we support spectators (of course we will), the add/removal of the
     * spectators need to happen outside
     */
    void initPlayers(
        std::unique_ptr<logic::PlayerManager> pm, std::unique_ptr<logic::ColonyManager> cm,
        std::map<uint64_t /*player_id*/, std::reference_wrapper<logic::Colony>>
            player_colony_map,
        uint64_t human_id);


    void initRecorder(std::unique_ptr<logic::InputRecorder> ir) { ir_ = std::move(ir); }

    /**
     * If you need to reproduce input, you add the reproducer here
     *
     * Besides the `update()` field, it will only be added here because both
     * instances (the game and reproducer) has the same lifetimes)
     */
    void initReproducer(std::unique_ptr<logic::InputReproducer> irepr) { irepr_ = std::move(irepr); }


    /**
     * Initialize the asset library
     */
    void initAssets();
    
    /**
     * Initialize the object factory, with all game objects, and return a reference to it
     *
     * This is good, so we can get the object checksums
     */
    logic::ObjectFactory* initObjectFactory();
    
    /**
     * Initialize the object manager
     */
    void initObjectManager();

    void initLoopData(uint64_t human_id);

    bool runLoop();

    /// Return maximum, minimum and average fps
    std::tuple<double, double, double> getStatisticInfo();

    logic::ObjectManager* getObjectManager() const;

    logic::PlayerManager* getPlayerManager() { return pm_.get(); }
    uint64_t getHumanPlayerID() { return human_id_; }
    
    
private:
    ///////////////////// logic

    /// The ID of the human player.
    /// Used to show what player to show selections, camera coordinates...
    uint64_t human_id_ = 0;

    std::unique_ptr<logic::TerrainFile> terrFile_;
    std::unique_ptr<logic::Terrain> terrain_;

    std::unique_ptr<logic::PlayerManager> pm_;

    std::vector<std::unique_ptr<logic::GameObject>> factory_objects_;
    
    std::map<uint64_t /*player_id*/, std::reference_wrapper<logic::Colony>> colonies_;

    std::unique_ptr<logic::ObjectManager> om_;
    std::unique_ptr<PreviewRenderer> pr_;
    std::unique_ptr<logic::ObjectLifecycleManager> olm_;
    std::unique_ptr<logic::ColonyManager> cm_;

    // might not be used at all, but it needs to have the same lifetime
    // as the game, so inputs can be captured.
    std::unique_ptr<logic::InputRecorder> ir_;

    std::unique_ptr<logic::InputReproducer> irepr_;

    
    std::chrono::duration<double, std::milli> delta{1.0};
    double pms = 0.0;

    double maxdelta = 0, mindelta = 99, sumfps = 0;
    bool started_ = false;

    // Run the logic engine at 60 Hz
#define LOGIC_DELTA 16

    // and the input engine at 120 Hz
#define INPUT_DELTA 8

    double logicTime = LOGIC_DELTA;
    double inputTime = INPUT_DELTA;
    int limax        = 0;
    std::chrono::high_resolution_clock::time_point ticks_;
    std::chrono::high_resolution_clock::time_point rendertime_;
    std::chrono::duration<double, std::milli> logictime_;
    std::chrono::duration<double, std::milli> inputtime_;
    std::chrono::duration<double, std::milli> drawtime_;

    // todo: probably will be removed?
    logic::GameActionManager gam;
    logic::GameContext gctx = {};

    ////////////////////// gfx
    graphics::Window* window_;
    graphics::Framebuffer* fb3D_;
    graphics::Framebuffer* fbGUI_;
    graphics::gui::GUIManager* gui_;
    std::unique_ptr<graphics::ObjectRenderer> objrend_;

    graphics::Renderer* rndr_ = nullptr;

    ////////////////////// more or less both
    std::unique_ptr<graphics::Camera> camera_;
    graphics::TerrainRenderer* terr_rend_ = nullptr;
    std::unique_ptr<graphics::AssetManager>& am;
    std::unique_ptr<graphics::SceneManager> scenernd_;

    ////////////////////////// input
    std::unique_ptr<input::InputPicker> ip_;

    /////////////////////// statistical + debug information
    struct {
        //  graphics::gui::GUIPanel *p;
        graphics::gui::Label* lbl;
        graphics::gui::Label* lblVersion;
        //  graphics::gui::GUIPanel *pnl;
        graphics::gui::Button* btn;

        graphics::gui::Label* lblBuilding   = nullptr;
        graphics::gui::Label* lblFPS        = nullptr;
        graphics::gui::Label* lblRange      = nullptr;
        graphics::gui::Label* lblSelected   = nullptr;
        graphics::gui::Label* lblTerrainPos = nullptr;
        graphics::gui::Label* lblKeys       = nullptr;
    } widgets;

    int frame_ = 0;

    bool runInput();

    void runLogic();
    void runGraphical(double framems);

    /* Show on-screen debug info
     * (aka the words in monospaced font you see in-game)
     */
    void showDebugInfo();

    void showHumanPlayerInfo(logic::Player*);
};
}  // namespace familyline
