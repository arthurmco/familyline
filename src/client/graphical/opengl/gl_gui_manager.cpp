#include <GL/glew.h>

#include <client/graphical/exceptions.hpp>
#include <client/graphical/window.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/opengl/gl_gui_manager.hpp>

using namespace familyline::graphics::gui;
using namespace familyline::graphics;
using namespace familyline::input;

/* The panel vertex square coordinates.
   It's a big rectangle, that fills the entire screen  */
static const float window_pos_coord[][3] = {
    {-1, 1, 1}, {-1, -1, 1}, {1, -1, 1}, {-1, 1, 1},
    {1, 1, 1},  {1, -1, 1}

    //  {-1, -1, 1}, { 1, -1, 1}, { 1, 1, 1},
    //  {-1, -1, 1}, {-1,  1, 1}, { 1, 1, 1}
};

/* Coordinates for every panel texture.
   Since they have the same vertex order, we don't need to declare multiple
   texture coordinates
   Also send the y coordinate inverted, because in OpenGL, Y positive is down, not up
*/
static const float window_texture_coord[][2] = {{-1, 1}, {-1, -1}, {1, -1},
                                                {-1, 1}, {1, 1},   {1, -1}};

/* Initialize shaders and window vertices.
 *
 * We render everything to a textured square. This function creates
 * the said square, plus the shaders that enable the rendering there
 */
void GLGUIManager::init(const Window& win)
{
    int win_w, win_h;

    /* Get window size */
    win.getSize(win_w, win_h);

    auto& d = GFXService::getDevice();

    sGUI_ = d->createShaderProgram(
        "gui", {d->createShader("shaders/GUI.vert", ShaderType::Vertex),
                d->createShader("shaders/GUI.frag", ShaderType::Fragment)});

    sGUI_->link();

    auto fnGetAttrib = [&](const char* name) {
        return glGetAttribLocation(sGUI_->getHandle(), name);
    };

    attrPos_ = fnGetAttrib("position");
    attrTex_ = fnGetAttrib("in_uv");

    //width_  = (unsigned)win_w;
    //height_ = (unsigned)win_h;

    // Create the vertices
    glGenVertexArrays(1, &(this->vaoGUI_));
    glBindVertexArray(this->vaoGUI_);

    /* Create vertex information */
    glGenBuffers(1, &(this->vboPos_));
    glBindBuffer(GL_ARRAY_BUFFER, this->vboPos_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 18, window_pos_coord, GL_STATIC_DRAW);
    glVertexAttribPointer(this->attrPos_, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(this->attrPos_);

    glGenBuffers(1, &(this->vboTex_));
    glBindBuffer(GL_ARRAY_BUFFER, this->vboTex_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, window_texture_coord, GL_STATIC_DRAW);
    glVertexAttribPointer(this->attrTex_, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(this->attrTex_);

    glBindVertexArray(0);

    /* Create texture where we'll render the canvas */
    glGenTextures(1, &texHandle_);
    glBindTexture(GL_TEXTURE_2D, texHandle_);

    LOGDEBUG(LoggerService::getLogger(), "gui-manager", "gui size: %dx%d", width_, height_);

    cairo_surface_flush(this->canvas_);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_BGRA, GL_UNSIGNED_BYTE,
        (void*)cairo_image_surface_get_data(this->canvas_));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        char e[128];
        snprintf(e, 127, "error %#x while setting texture for GUI content", err);
        throw graphical_exception(std::string(e));
    }
}

/**
 * Render the cairo canvas to the gui texture
 */
void GLGUIManager::renderToTexture()
{
    auto& log = LoggerService::getLogger();

    // Make the GUI texture transparent
    glClearColor(0.0, 0.0, 0.0, 0.0);
    GLint depthf;
    glGetIntegerv(GL_DEPTH_FUNC, &depthf);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    GFXService::getShaderManager()->use(*sGUI_);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texHandle_);

    sGUI_->setUniform("texPanel", 0);
    sGUI_->setUniform("opacity", 1.0f);

    cairo_surface_flush(this->canvas_);
    auto* canvas_data = cairo_image_surface_get_data(this->canvas_);

    glBindVertexArray(this->vaoGUI_);

    glEnableVertexAttribArray(attrPos_);
    glBindBuffer(GL_ARRAY_BUFFER, vboPos_);
    glVertexAttribPointer(attrPos_, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(attrTex_);
    glBindBuffer(GL_ARRAY_BUFFER, vboTex_);
    glVertexAttribPointer(attrTex_, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindTexture(GL_TEXTURE_2D, this->texHandle_);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, this->width_, this->height_, 0, GL_BGRA, GL_UNSIGNED_BYTE,
        canvas_data);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        log->write("gui-renderer", LogType::Error, "OpenGL error %#x", err);
    }

    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDepthFunc(depthf);
    glDisable(GL_BLEND);

    glClearColor(0.0, 0.0, 0.0, 1.0);
}
