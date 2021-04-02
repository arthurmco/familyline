#include <client/graphical/opengl/gl_device.hpp>

#ifdef RENDERER_OPENGL

#include <fmt/core.h>
#include <SDL2/SDL_opengl.h>

#include <client/graphical/opengl/gl_window.hpp>
#include <client/graphical/opengl/gl_shader.hpp>
#include <client/graphical/opengl/gl_framebuffer.hpp>

using namespace familyline;
using namespace familyline::graphics;

/**
 * Get a list of devices in the machine
 */
std::vector<std::unique_ptr<Device>> GLDevice::getDeviceList()
{
    // TODO: make this a static method of each device?
    std::vector<std::unique_ptr<Device>> devs;

    auto r = SDL_Init(SDL_INIT_EVERYTHING);
    if (r != 0) {
    }

    auto driverCount = SDL_GetNumVideoDrivers();
    for (auto i = 0; i < driverCount; i++) {
        auto code       = SDL_GetVideoDriver(i);
        auto is_default = !strcmp(code, SDL_GetCurrentVideoDriver());

        devs.push_back(std::make_unique<GLDevice>(code, is_default, i));
    }

    return devs;
}

GLDevice::GLDevice(const char* code, bool is_default, int index)
{
    _code    = std::string_view(code);
    _default = is_default;
    _data    = std::any(index);
}

/// Get the device name
std::string_view GLDevice::getCode() { return _code; }

/// Get the device name
std::string_view GLDevice::getName()
{
    const unsigned char* s = glGetString(GL_RENDERER);

    if (s)
        return std::string_view((const char*)s);
    else
        return std::string_view("unknown");
}

/// Get the device name
std::string_view GLDevice::getVendor()
{
    const unsigned char* s = glGetString(GL_VENDOR);

    if (s)
        return std::string_view((const char*)s);
    else
        return std::string_view("you need to call this after initialize opengl");
}

bool GLDevice::isDefault() { return _default; }

std::any GLDevice::getCustomData() { return _data; }

familyline::graphics::Window* GLDevice::createWindow(size_t w, size_t h)
{
    return new GLWindow(this, w, h);
}

Shader* GLDevice::createShader(const char* file, ShaderType type) { return new GLShader{file, type}; }

ShaderProgram* GLDevice::createShaderProgram(
    std::string_view name, std::initializer_list<Shader*> shaders)
{
    std::vector<GLShader*> glsh;
    std::transform(shaders.begin(), shaders.end(), std::back_inserter(glsh), [](Shader* s) {
        return dynamic_cast<GLShader*>(s);
    });

    return new GLShaderProgram{name, glsh};
}


Framebuffer* GLDevice::createFramebuffer(std::string name, int width, int height)
{
    return new GLFramebuffer(name, width, height);
}


#endif
