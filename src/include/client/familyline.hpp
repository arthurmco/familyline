#pragma once
/***
    Familyline main file header

    Contains some data structures

    Copyright 2021 Arthur Mendes.

***/

#ifdef _WIN32
#define _WINSOCKAPI_
#define _WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ws2tcpip.h>
#define usleep(x) Sleep(x / 1000);
#define sleep(x) Sleep(x * 1000);
#endif

#define GLM_FORCE_RADIANS

#include <cinttypes>
#include <client/HumanPlayer.hpp>
#include <client/config_reader.hpp>
#include <client/game.hpp>
#include <client/graphical/device.hpp>
#include <client/graphical/framebuffer.hpp>
#include <client/graphical/gui/gui_button.hpp>
#include <client/graphical/gui/gui_checkbox.hpp>
#include <client/graphical/gui/gui_container_component.hpp>
#include <client/graphical/gui/gui_imageview.hpp>
#include <client/graphical/gui/gui_label.hpp>
#include <client/graphical/gui/gui_listbox.hpp>
#include <client/graphical/gui/gui_manager.hpp>
#include <client/graphical/gui/gui_textbox.hpp>
#include <client/graphical/gui/gui_window.hpp>
#include <client/graphical/renderer.hpp>
#include <client/graphical/shader_manager.hpp>
#include <client/graphical/window.hpp>
#include <client/input/InputPicker.hpp>
#include <client/input/input_service.hpp>
#include <client/loop_runner.hpp>
#include <client/params.hpp>
#include <client/player_enumerator.hpp>
#include <common/logger.hpp>
#include <common/logic/input_recorder.hpp>
#include <common/logic/input_reproducer.hpp>
#include <common/net/game_packet_server.hpp>
#include <common/net/network_client.hpp>
#include <common/net/server.hpp>
#include <common/net/server_finder.hpp>

#ifdef WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno

#include <sys/utsname.h>
#include <unistd.h>
#endif

#ifdef _MSC_VER
#undef main  // somehow vs does not find main()

#endif

namespace familyline
{
/**
 * Information required to start a game, besides player information
 */
struct StartGameInfo {
    /// The terrain file
    std::string mapFile;

    /// If present, the input record file.
    /// If this is present, it means that we have a recorded game.
    std::optional<std::string> inputFile;
};

/**
 * A structure with all the data needed to draw something
 */
struct GraphicalInfo {
    graphics::Framebuffer* f3D;
    graphics::Framebuffer* fGUI;
    graphics::Window* win;
    graphics::gui::GUIManager* guir;
    size_t gwidth;
    size_t gheight;
};

}  // namespace familyline
