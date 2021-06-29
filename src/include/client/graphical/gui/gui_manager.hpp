#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <client/graphical/gui/gui_button.hpp>
#include <client/graphical/gui/gui_imageview.hpp>
#include <client/graphical/gui/gui_label.hpp>
#include <client/graphical/gui/gui_window.hpp>
#include <client/graphical/gui/root_control.hpp>
#include <client/graphical/shader.hpp>
#include <client/input/input_manager.hpp>
#include <common/logger.hpp>
#include <deque>
#include <memory>
#include <queue>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace familyline::graphics
{
class Window;
}

namespace familyline::graphics::gui
{
/// TODO: PLEASE find a way to run the GUI update function always in tribalia.cpp, or
///       familyline.cpp if renamed. It will make things easier.

/**
 * Manages the graphical interface state and rendering
 */
class GUIManager
{
protected:
    familyline::graphics::Window& win_;

    unsigned width_, height_;

    // Cairo things, to actually do the rendering work
    // Since Cairo can use hardware acceleration, we do not need to worry much about speed.
    cairo_t* context_;
    cairo_surface_t* canvas_;

    std::unique_ptr<RootControl> root_control_;

    // Cached mouse positions, to help check events that do not send the mouse position along them,
    // like the KepayEvent
    int hitmousex_ = 1, hitmousey_ = 1;
    std::queue<familyline::input::HumanInputAction> input_actions_;

    bool destroying = false;
    CallbackQueue cb_queue_;

    /// Hovered control
    std::optional<Control*> hovered_ = std::nullopt;

    /// A map of all windows created, keyed by name
    std::unordered_map<std::string, std::unique_ptr<GUIWindow>> windows_;

    /// TODO: add a way to lock event receiving to the GUI. Probably the text edit control
    /// will need, to ensure you can type on it when you click and continue to be able to,
    /// even if you move the mouse out of it.

    /**
     * Initialize shaders and window vertices.
     *
     * We render everything to a textured square. This function creates
     * the said square, the texture plus the shaders that enable the
     * rendering there
     */
    virtual void init(const familyline::graphics::Window& win) = 0;

    /**
     * Render the cairo canvas to the gui texture
     */
    virtual void renderToTexture() = 0;

    /**
     * Checks if an event mouse position hits a control or not.
     * This allows us to ignore events that do not belong to us, and pass them
     * to the handlers under it, such as the ones that handle game input
     *
     * If it did not hit any control, return false, else return true
     */
    bool checkIfEventHits(const familyline::input::HumanInputAction&);

    /**
     * Get the control that is at the specified pixel coordinate
     */
    std::optional<Control*> getControlAtPoint(int x, int y);

    struct GUIWindowInfo {
        GUIWindow* win;
        cairo_t* context;
        cairo_surface_t* canvas;
        
        
        GUIWindowInfo(GUIWindow* w, cairo_t* ctxt, cairo_surface_t* s)
            : win(w), context(ctxt), canvas(s)
        {
        }

        ~GUIWindowInfo()
        {
            cairo_surface_destroy(canvas);
            cairo_destroy(context);
        }

        GUIWindowInfo(const GUIWindowInfo& other) = delete;

        GUIWindowInfo(GUIWindowInfo&& other) noexcept
            : win(std::exchange(other.win, nullptr)),
              context(std::exchange(other.context, nullptr)),
              canvas(std::exchange(other.canvas, nullptr))
        {
        }

        GUIWindowInfo& operator=(const GUIWindowInfo& other) = delete;

        GUIWindowInfo& operator=(GUIWindowInfo&& other) noexcept
        {
            std::swap(win, other.win);
            std::swap(context, other.context);
            std::swap(canvas, other.canvas);
            return *this;
        }
    };

    /**
     * We need a location to plot text debug information (like fps, diagnostic information, tick
     * number) as we are playing the game.
     *
     * This debug window serves as a way to do this.
     * It will not receive any input events, but it will be the topmost window any time
     */
    GUIWindow debug_window_;
    GUIWindowInfo debug_window_info_;

    std::deque<GUIWindowInfo> windowstack_;

public:
    GUIManager(
        familyline::graphics::Window& win, unsigned width, unsigned height,
        familyline::input::InputManager& manager)
        : win_(win),
          width_(width),
          height_(height),
          debug_window_(width, height),
          debug_window_info_(&debug_window_, nullptr, nullptr),
          canvas_(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height))
    {
        context_ = cairo_create(this->canvas_);

        auto* canvas  = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        auto* context = cairo_create(canvas);
        debug_window_.modifyAppearance([](ControlAppearance& ca) {
            ca.background = {0, 0, 0, 0.0};
        });
        debug_window_.event_onDelete = [](auto& o) {};  // set the delete callback to avoid a
                                                        // bad function call exception on the
                                                        // destructor
        debug_window_info_ = GUIWindowInfo(&debug_window_, context, canvas);

        root_control_ = std::make_unique<RootControl>(width, height);

        manager.addListenerHandler([&](familyline::input::HumanInputAction i) {
            if (this->checkIfEventHits(i)) {
                input_actions_.push(i);
                return true;
            }

            return false;
        });
    }

    void initialize(const Window& win) { this->init(win); }

    /**
     * Show the window you pass
     *
     * Since the window have a "hidden" delete event, you do not need to remove the
     * window from the gui, deallocating it will be sufficient
     */
    void showWindow(GUIWindow* w);

    void closeWindow(const GUIWindow& w);

    // void add(int x, int y, std::unique_ptr<Control> control);
    // void add(double x, double y, ControlPositioning cpos, std::unique_ptr<Control> control);

    // void remove(Control* control);

    GUIWindow& getDebugWindow() { return debug_window_; }

    void update();

    void render(unsigned int x, unsigned int y);

    /**
     * Receive an event and act on it
     *
     * (In the game, we will probably use input actions, not sdl events directly)
     */
    void receiveEvent();

    /**
     * Check if we have any pending callbacks
     *
     * If we have, run them
     *
     * Note that this function needs to be ran in the main thread, because the game loop
     * runs in one of those callbacks, the new game button. This can cause some race
     * conditions and crash the game.
     *
     */
    void runCallbacks();

    /**
     * Creates a GUI window
     *
     * This window will be owned by the GUIManager, so you do not need
     * to care about destroying it
     */
    GUIWindow* createGUIWindow(std::string name, unsigned width, unsigned height);

    /**
     * Get the window named by `name`, or null if it does not exist
     *
     * This raw pointer might not always be valid in the lifetime of the window.
     */
    GUIWindow* getGUIWindow(std::string name);

    /**
     * Destroy window, aka remove it from memory
     *
     * This will also destroy all controls from it, since they are owned by the window.
     */
    void destroyGUIWindow(std::string name);

    virtual ~GUIManager();
};

}  // namespace familyline::graphics::gui
