#pragma once

#include <client/graphical/shader.hpp>

class TestShaderProgram : public familyline::graphics::ShaderProgram
{
private:
    int handle_ = -1;

public:
    TestShaderProgram(std::string_view name)
        : ShaderProgram(name),
          handle_(int((long int)name.data() + ((long int)(name.data()) >> 32)))
        {}

    virtual void link() {}

    virtual int getHandle() const { return handle_; }

    virtual void setUniform(std::string_view name, glm::vec3 val) {}
    virtual void setUniform(std::string_view name, glm::vec4 val) {}
    virtual void setUniform(std::string_view name, glm::mat4 val) {}
    virtual void setUniform(std::string_view name, int val) {}
    virtual void setUniform(std::string_view name, float val) {}

    virtual ~TestShaderProgram() {}

    virtual void use() {}

};
