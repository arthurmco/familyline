#pragma once


/***
    OpenGL device enumeration routine
    Due to the way this game handles OpenGL, it will only select the main "driver".    

    Copyright 2020 Arthur Mendes.

***/

#include <client/graphical/opengl/gl_headers.hpp>
#include <memory>

#ifdef RENDERER_OPENGL
#include <client/graphical/device.hpp>

namespace familyline::graphics
{
    class Window;
    class ShaderProgram;

class GLDevice : public Device
{
private:
    std::string_view _code;
    bool _default;
    std::any _data;

public:
    GLDevice(const char* code, bool is_default, int index);

    /// Get the device code, name and vendor
    virtual std::string_view getCode();
    virtual std::string_view getName();
    virtual std::string_view getVendor();

    virtual bool isDefault();

    virtual std::any getCustomData();

    virtual Window* createWindow(size_t w, size_t h);

    virtual Shader* createShader(const char* file, ShaderType type);
    virtual ShaderProgram* createShaderProgram(std::string_view name,
                                               std::initializer_list<Shader*> shaders);

    virtual Framebuffer* createFramebuffer(std::string name, int width, int height);

    /**
     * Get a list of devices in the machine
     */
    static std::vector<std::unique_ptr<Device>> getDeviceList();

    virtual ~GLDevice() {}
};
}  // namespace familyline::graphics

#endif
