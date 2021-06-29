#include <client/graphical/exceptions.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/shader_manager.hpp>

using namespace familyline::graphics;

void ShaderManager::use(ShaderProgram& s)
{
    if (lastShader == s.getHandle()) return;

    s.use();
    lastShader = s.getHandle();
}

void ShaderManager::addShader(ShaderProgram* s) { _shaders[std::string{s->getName()}] = s; }

ShaderProgram* ShaderManager::getShader(std::string_view name)
{
    auto res = _shaders.find(std::string{name});
    if (res == _shaders.end()) {
        char s[256];        
        sprintf(s, "Shader %s not found in shader manager", name.data());
        throw shader_exception(std::string_view{s}, 2, std::string{name});
    }

    return res->second;
}
