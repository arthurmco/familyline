#pragma once

/**
 * Those classes below mocks a device, plus a list of shaders
 *
 * They are useful to test some aspects of the graphical system without
 * calling any specific API
 */

#include <any>
#include <client/graphical/device.hpp>
#include <client/graphical/shader.hpp>
#include <client/graphical/gfx_service.hpp>

class TestShader : public familyline::graphics::Shader
{
private:
    int idx = 0;

public:
    TestShader(const char* file, ShaderType type);

    virtual void compile() {}

    virtual int getHandle() const { return idx; }

    virtual ~TestShader() {}
};

class TestShaderProgram : public familyline::graphics::ShaderProgram
{
public:
    TestShaderProgram(std::string_view name) : ShaderProgram(name) {}

    virtual void link() {}

    virtual int getHandle() const
    {
        return int(uintptr_t(_name.data())) + int(uintptr_t(_name.data()) >> 32);
    }

    virtual void setUniform(std::string_view name, glm::vec3 val) {}
    virtual void setUniform(std::string_view name, glm::vec4 val) {}
    virtual void setUniform(std::string_view name, glm::mat4 val) {}
    virtual void setUniform(std::string_view name, int val) {}
    virtual void setUniform(std::string_view name, float val) {}

    virtual ~TestShaderProgram() {}

    virtual void use() {}
};

class TestDevice : public familyline::graphics::Device
{
private:
    std::unique_ptr<TestShaderProgram> fwd = std::make_unique<TestShaderProgram>("forward");        
public:
    TestDevice() {
        auto& sm = familyline::graphics::GFXService::getShaderManager();
        sm->addShader(fwd.get());
    }
    
    /// Get the device code, name and vendor
    virtual std::string_view getCode() { return "0000TEST"; }
    virtual std::string_view getName() { return "TestName"; }
    virtual std::string_view getVendor() { return "TestVendor"; }

    virtual bool isDefault() { return true; }

    /// Get data that wil only make sense to the API, or to the respective
    /// window (ex: a GLDevice data will only make sense to a GLWindow,
    /// a VulkanDevice to a Vulkan Window...)
    virtual std::any getCustomData() { return std::any{}; }

    virtual familyline::graphics::Shader* createShader(const char* file, ShaderType type)
    {
        return new TestShader{file, type};
    }
    virtual familyline::graphics::ShaderProgram* createShaderProgram(
        std::string_view name, std::initializer_list<familyline::graphics::Shader*> shaders)
    {
        std::vector<TestShader*> glsh;
        std::transform(
            shaders.begin(), shaders.end(), std::back_inserter(glsh),
            [](familyline::graphics::Shader* s) { return dynamic_cast<TestShader*>(s); });

        return new TestShaderProgram{name};
    }

    virtual familyline::graphics::Framebuffer* createFramebuffer(
        std::string name, int width, int height);

    virtual familyline::graphics::Window* createWindow(size_t w, size_t h) { return nullptr; }

    virtual ~TestDevice() {};

};
