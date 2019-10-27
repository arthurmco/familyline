/*
  Game loop class

  Copyright (C) 2017, 2019 Arthur Mendes

*/

#ifndef _GAME_HPP
#define _GAME_HPP

#include "config.h"
#include "EnviroDefs.h"

//#include "logic/ObjectRenderer.hpp"
#include "logic/PathFinder.hpp"
#include "logic/ObjectFactory.hpp"
#include "logic/ObjectPathManager.hpp"
#include "logic/TerrainFile.hpp"
#include "logic/Team.hpp"
#include "logic/BuildQueue.hpp"
#include "logic/GameActionManager.hpp"
#include "logic/PlayerManager.hpp"
#include "logic/debug_drawer.hpp"

#include "graphical/camera.hpp"
#include "graphical/asset_file.hpp"
#include "graphical/Light.hpp"
#include "graphical/meshopener/OBJOpener.hpp"
#include "graphical/materialopener/MTLOpener.hpp"
#include "graphical/TerrainRenderer.hpp"
#include "graphical/TextureOpener.hpp"
#include "graphical/TextureManager.hpp"
#include "graphical/MaterialManager.hpp"
#include "graphical/asset_manager.hpp"
#include "graphical/window.hpp"
#include "graphical/framebuffer.hpp"
#include "graphical/object_renderer.hpp"
#include "graphical/renderer.hpp"
#include "graphical/scene_renderer.hpp"
#include "graphical/GraphicalPlotInterface.hpp"
#include "graphical/gfx_service.hpp"
#include "graphical/gui/GUIManager.hpp"
#include "graphical/gui/GUILabel.hpp"
#include "graphical/gui/GUIButton.hpp"
//#include "graphical/gui/ImageControl.hpp"


#include "input/InputPicker.hpp"

#include "Log.hpp"
#include "Timer.hpp"
#include "HumanPlayer.hpp"
#include "objects/WatchTower.hpp"
#include "objects/Tent.hpp"

namespace familyline {
    
    class Game {
    private:
        graphics::Window* win = nullptr;
        graphics::Renderer* rndr = nullptr;
        graphics::SceneRenderer* scenernd = nullptr;
        graphics::Framebuffer *fbGUI = nullptr, *fb3D = nullptr;
        graphics::gui::GUIManager* gr = nullptr;
        logic::PlayerManager* pm = nullptr;
        logic::ObjectManager* om = nullptr;
        
        HumanPlayer* hp = nullptr;
        logic::Terrain* terr = nullptr;
        logic::TerrainFile* terrFile;
    
        logic::GameContext gctx;
        logic::GameActionManager gam;
    
    
        bool player = false;

        graphics::Camera* cam;

        std::unique_ptr<graphics::AssetManager>& am = graphics::GFXService::getAssetManager();

        graphics::TerrainRenderer* terr_rend = nullptr;
        graphics::ObjectRenderer* objrend = nullptr;

        input::InputPicker* ip = nullptr;
        logic::PathFinder* pathf = nullptr;

        struct {
            //  graphics::gui::GUIPanel *p;
            graphics::gui::GUILabel *lbl;
            graphics::gui::GUILabel *lblVersion;
            //  graphics::gui::GUIPanel *pnl;
            graphics::gui::GUIButton* btn;
        } widgets;


        /* Run input-related code
         * Return false if the player asked to exit the game.
         */
        bool RunInput();
    
        void RunLogic();
        void RunGraphical();

        /* Show on-screen debug info
         * (aka the words in monospaced font you see in-game)
         */
        void ShowDebugInfo();

    public:
        Game(graphics::Window* w, graphics::Framebuffer* fb3D,
             graphics::Framebuffer* fbGUI, graphics::gui::GUIManager* gr,
             logic::PlayerManager* pm, HumanPlayer* hp);

        int RunLoop();
    };
    
}

#endif



