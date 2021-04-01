#pragma once

#include <client/graphical/opengl/gl_headers.hpp>

#include <client/graphical/gui/gui_manager.hpp>
#include <client/graphical/shader.hpp>
#include <common/logger.hpp>

namespace familyline::graphics::gui
{
/// TODO: PLEASE find a way to run the GUI update function always in tribalia.cpp, or
///       familyline.cpp if renamed. It will make things easier.

/**
 * Manages the graphical interface state and rendering
 */
class GLGUIManager : GUIManager
{
private:
    familyline::graphics::ShaderProgram* sGUI_;
    GLuint vaoGUI_, attrPos_, vboPos_, attrTex_, vboTex_, texHandle_;

    /**
     * Initialize shaders and window vertices.
     *
     * We render everything to a textured square. This function creates
     * the said square, the texture plus the shaders that enable the
     * rendering there
     */
    virtual void init(const familyline::graphics::Window& win);

    /**
     * Render the cairo canvas to the gui texture
     */
    virtual void renderToTexture();

public:
    GLGUIManager(
        familyline::graphics::Window& win, unsigned width, unsigned height,
        familyline::input::InputManager& manager)
        :
        GUIManager(win, width, height, manager)
        {}
        

    virtual ~GLGUIManager() {
        if (sGUI_) {
            delete sGUI_;
        }        
    };
};

}  // namespace familyline::graphics::gui
