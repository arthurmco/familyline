#include <SDL2/SDL_messagebox.h>
#include <fmt/core.h>

#include <client/graphical/opengl/gl_window.hpp>

#include "client/graphical/gui/gui_manager.hpp"
#include "client/graphical/window.hpp"

#ifdef RENDERER_OPENGL

#include <client/graphical/exceptions.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/opengl/gl_renderer.hpp>
#include <client/graphical/opengl/gl_shader.hpp>
#include <client/graphical/renderer.hpp>
#include <client/graphical/shader_manager.hpp>
#include <client/input/input_service.hpp>
#include <common/logger.hpp>

using namespace familyline::graphics;

/**
 * Enable OpenGL debugging messages
 *
 * Need to be called after you start GLEW, or this function will not work correctly,
 * because it depends from opengl extensions
 */
static void enable_gl_debug()
{
    using namespace familyline;

    auto& log = LoggerService::getLogger();
    struct LogTime {
        unsigned qt      = 0;
        unsigned lastsec = 0;
    };

    auto gl_debug_callback = [](GLuint source, GLuint type, unsigned int id, GLuint severity,
                                int length, const char* msg, const void* userparam) {
        (void)userparam;

        /* Handle log suppressing */
        static std::map<unsigned, LogTime> id_qt_map;
        static unsigned lastsupp = (unsigned)-1;
        auto t                   = time(NULL);
        auto& log                = LoggerService::getLogger();

        /* Unban after a long time */
        if (id == lastsupp && id_qt_map[id].lastsec < t + 5) {
            id_qt_map[id].qt = 0;
            return;
        } else {
            lastsupp = -1;
        }

        if (t >= id_qt_map[id].lastsec) {
            id_qt_map[id].qt++;
            id_qt_map[id].lastsec = t;
        }

        if (id_qt_map[id].qt > 20 && id_qt_map[id].lastsec <= t) {
            log->write(
                "gl-debug-output", LogType::Warning,
                "Suppressing id %x messages because they are too many", id);
            lastsupp = id;
            return;
        }

        id_qt_map[id].lastsec = t;

        /* Handle message parsing and display */
        const char *ssource, *stype, *sseverity;
        switch (source) {
            case GL_DEBUG_SOURCE_API: ssource = "gl-debug-opengl-api"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM: ssource = "gl-debug-window-system"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: ssource = "gl-debug-shader-compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY: ssource = "gl-debug-third-party"; break;
            case GL_DEBUG_SOURCE_APPLICATION: ssource = "gl-debug-application"; break;
            case GL_DEBUG_SOURCE_OTHER: ssource = "gl-debug-other"; break;
            default: ssource = "gl-debug"; break;
        }

        switch (type) {
            case GL_DEBUG_TYPE_ERROR: stype = "error"; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: stype = "deprecated behavior"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: stype = "undefined behavior"; break;
            case GL_DEBUG_TYPE_PORTABILITY: stype = "portability issue"; break;
            case GL_DEBUG_TYPE_PERFORMANCE: stype = "performance"; break;
            case GL_DEBUG_TYPE_OTHER: stype = "other"; break;
            default: stype = "unknown"; break;
        }

        LogType ltype = LogType::Info;

        bool do_exit = false;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                sseverity = "H";
                ltype     = LogType::Error;
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                ltype     = LogType::Warning;
                sseverity = "M";
                break;
            case GL_DEBUG_SEVERITY_LOW:
                ltype     = LogType::Info;
                sseverity = "L";
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                ltype     = LogType::Debug;
                sseverity = "N";
                break;
            default: do_exit = true; break;
        }

        if (do_exit) {
            return;
        }

        char* smsg = new char[length + 2];
        memset(smsg, 0, length + 2);
        strncpy(smsg, msg, length);

        fmt::memory_buffer out;
        format_to(out, "[#{:x}] {:s} {:s}: {:s}\0\0", id, sseverity, stype, smsg);

        delete[] smsg;
        LoggerService::getLogger()->write(ssource, ltype, "%s", fmt::to_string(out).data());
    };

    glEnable(GL_DEBUG_OUTPUT);

    if (GL_KHR_debug && glDebugMessageCallback) {
        // Try KHR_debug first
        log->write(
            "gl_window", LogType::Info, "KHR_debug supported and used to get GPU debug messages");
        glDebugMessageCallback(gl_debug_callback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    } else if (GLEW_ARB_debug_output && glDebugMessageCallbackARB) {
        // Try ARB_debug_output as a fallback (it's older, but best supported)
        log->write(
            "gl_window", LogType::Info,
            "ARB_debug_output supported and used to get GPU debugging messages");
        /* Create the callback */
        glDebugMessageCallbackARB(gl_debug_callback, nullptr);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    } else {
        log->write(
            "gl_window", LogType::Warning,
            "Neither KHR_debug nor ARB_debug_output extensions are supported");
        log->write("gl_window", LogType::Warning, "GPU debugging messages will not be available");
    }
}

GLWindow::GLWindow(GLDevice* dev, int width, int height) : _dev(dev), _width(width), _height(height)
{
    /* Setup SDL GL context data */
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    // Needed to run in macOS
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    auto fflags = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &fflags);
    fflags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;

    if (true) {
        fflags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, fflags);
    _win = SDL_CreateWindow(
        "Familyline", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width, _height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

    if (!_win) {
        auto err = std::string("OpenGL context creation error: ");
        err.append(SDL_GetError());

        SDL_Quit();
        throw graphical_exception(err);
    }
}

void GLWindow::getSize(int& width, int& height) const
{
    width  = _width;
    height = _height;
}

/* Get the window framebuffer size
 *
 * This value can differ from the window size if the
 * OS do some sort of dpi scaling
 *
 * macOS is one example
 */
void GLWindow::getFramebufferSize(int& width, int& height)
{
    width  = _fwidth;
    height = _fheight;
}

void GLWindow::show()
{
    /* Create the context */
    _glctxt = SDL_GL_CreateContext(_win);

    if (_glctxt == NULL) {
        auto err = std::string("OpenGL context creation error: ");
        err.append(SDL_GetError());
        SDL_Quit();
        throw renderer_exception(err, 0);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glewExperimental  = GL_TRUE;
    GLenum glewStatus = glewInit();

    /* glewInit() can emit GL_INVALID_ENUM, but this doesn't seem to affect
       the operation of the library
       (check https://www.khronos.org/opengl/wiki/OpenGL_Loading_Library) */

    glGetError();

    if (glewStatus != GLEW_OK) {
        auto err = std::string("OpenGL context creation error: ");
        err.append(SDL_GetError());
        SDL_Quit();
        throw renderer_exception(err, glewStatus);
    }

    SDL_GL_GetDrawableSize(_win, &_fwidth, &_fheight);
    printf("apparent window size is %d x %d\n", _fwidth, _fheight);

    this->createWindowSquare();
    SDL_ShowWindow(_win);

    enable_gl_debug();
}

static const GLfloat base_win_square_points[] = {-1.0, 1.0,  1.0, 1.0,  1.0,  1.0,
                                                 1.0,  -1.0, 1.0, -1.0, -1.0, 1.0};

static const GLuint base_win_square_elements[] = {0, 3, 2, 0, 1, 2};

void GLWindow::createWindowSquare()
{
    /* Compile the shader */
    if (!winShader) {
        winShader = new GLShaderProgram(
            "window", {new GLShader("shaders/Window.vert", ShaderType::Vertex),
                       new GLShader("shaders/Window.frag", ShaderType::Fragment)});

        winShader->link();
    }

    /* Create the basic VAO for the window */

    glGenVertexArrays(1, &base_vao);
    glBindVertexArray(base_vao);

    glGenBuffers(1, &base_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, base_vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(base_win_square_points), base_win_square_points, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &base_index_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, base_index_vbo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, sizeof(base_win_square_elements), base_win_square_elements,
        GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void GLWindow::setFramebuffers(Framebuffer* f3D, Framebuffer* fGUI)
{
    _f3D  = f3D;
    _fGUI = fGUI;
}

void GLWindow::update()
{
    glClearColor(0, 1.0, 0, 1.0);
    glViewport(0, 0, _fwidth, _fheight);

    glDisable(GL_DEPTH_TEST);
    GFXService::getShaderManager()->use(*winShader);

    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    if (_f3D) glBindTexture(GL_TEXTURE_2D, _f3D->getTextureHandle());

    glActiveTexture(GL_TEXTURE1);
    if (_fGUI) glBindTexture(GL_TEXTURE_2D, _fGUI->getTextureHandle());

    winShader->setUniform("texRender", 0);
    winShader->setUniform("texGUI", 1);

    glBindVertexArray(base_vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, base_vbo);
    glVertexAttribPointer(
        0,         // attribute 0.
        3,         // size
        GL_FLOAT,  // type
        GL_FALSE,  // normalized?
        0,         // stride
        (void*)0   // array buffer offset
    );

    // Draw the screen triangle
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, base_index_vbo);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    SDL_GL_SwapWindow(_win);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        switch (err) {
            case GL_OUT_OF_MEMORY: throw graphical_exception("Out of memory while rendering");
            default:
                LoggerService::getLogger()->write("window", LogType::Error, "GL error %#x\n", err);
        }
    }

    glDisableVertexAttribArray(0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_DEPTH_TEST);
}

gui::GUIManager* GLWindow::createGUIManager()
{
    if (!guim_.get()) {
        int gwidth, gheight;
        this->getFramebufferSize(gwidth, gheight);

        auto& ima = input::InputService::getInputManager();

        guim_ = std::make_unique<gui::GLGUIManager>(*this, gwidth, gheight, *ima.get());
    }

    return (gui::GUIManager*)guim_.get();
}

Renderer* GLWindow::createRenderer()
{
    renderer_ = std::unique_ptr<Renderer>((Renderer*)new GLRenderer{});
    return (GLRenderer*)renderer_.get();
}
Renderer* GLWindow::getRenderer() { return (GLRenderer*)renderer_.get(); }

void GLWindow::showMessageBox(std::string title, SysMessageBoxFlags flags, std::string content)
{
    uint32_t sdlflags = 0;
    switch (flags) {
        case SysMessageBoxFlags::Error: sdlflags = SDL_MESSAGEBOX_ERROR; break;
        case SysMessageBoxFlags::Warning: sdlflags = SDL_MESSAGEBOX_WARNING; break;
        case SysMessageBoxFlags::Information: sdlflags = SDL_MESSAGEBOX_INFORMATION; break;
    }

    int buttonid = 0;
    if (SDL_ShowSimpleMessageBox(
            sdlflags, title.c_str(), content.c_str(), this->_win ? this->_win : nullptr) < 0) {
        fmt::print("{}\n{}\n\n", title, content);
    }
}

#endif
