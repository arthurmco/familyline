#pragma once

#include <client/graphical/opengl/gl_headers.hpp>

#ifdef RENDERER_OPENGL

#include <client/graphical/shader.hpp>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace familyline::graphics
{
/**
 * Represents a shader file
 *
 * We unite those shader files in a shader program.
 * @see ShaderProgram
 */
class GLShader : public Shader
{
private:
    int _handle;

    std::string readAndProcessFile(const char* file);    
    std::string readFile(const char* file);

public:
    GLShader(const char* file, ShaderType type);
    virtual void compile();

    virtual int getHandle() const { return this->_handle; }   
};

    
class GLShaderProgram : public ShaderProgram
{
private:
    std::vector<std::pair<ShaderType, GLShader*>> _files;
    int _handle;
    std::unordered_map<std::string, GLint> _uniform_cache;

    GLint getUniformLocation(std::string_view name);

public:    
    GLShaderProgram(std::string_view name, const std::vector<GLShader*>& shaders);
    virtual void link();

    virtual int getHandle() const { return this->_handle; }
    virtual std::string_view getName() const { return _name; }

    virtual void setUniform(std::string_view name, glm::vec3 val);
    virtual void setUniform(std::string_view name, glm::vec4 val);
    virtual void setUniform(std::string_view name, glm::mat4 val);
    virtual void setUniform(std::string_view name, int val);
    virtual void setUniform(std::string_view name, float val);

    virtual void use();

    virtual ~GLShaderProgram() {
        for (auto& [_, s] : _files) {
            delete s;
        }
    }
};
}  // namespace familyline::graphics

#endif
