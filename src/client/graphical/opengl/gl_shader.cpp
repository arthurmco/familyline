#include <client/graphical/opengl/gl_shader.hpp>

#ifdef RENDERER_OPENGL

#include <client/graphical/exceptions.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/shader_manager.hpp>
#include <cstdio>

#include <regex>
#include <fmt/format.h>

using namespace familyline::graphics;

// Open the shader file and compile it
GLShader::GLShader(const char* file, ShaderType type)
    : Shader(file, type)
{
    auto content = this->readFile(file);

    GLenum gltype = -1;
    switch (type) {
        case ShaderType::Vertex:
            gltype = GL_VERTEX_SHADER;
            break;
        case ShaderType::Fragment:
            gltype = GL_FRAGMENT_SHADER;
            break;
        case ShaderType::Geometry:
            gltype = GL_GEOMETRY_SHADER;
            break;
        case ShaderType::Compute:
            gltype = GL_COMPUTE_SHADER;
            break;
    }

    this->_handle     = glCreateShader(gltype);
    const char* cdata = content.c_str();

    glShaderSource(this->_handle, 1, (const char**)&cdata, NULL);

    this->_file = file;
    this->_type = type;

    this->compile();
}

std::regex regInclude(R"(\#include\s*\"(.*)\")");

/**
 * Read and process files
 *
 * Deal with the include files here, too
 */
std::string GLShader::readAndProcessFile(const char* file)
{
    std::string data;

    std::string_view svfile(file);
    std::string_view basePath = svfile.substr(0, svfile.find_last_of('/'));

    FILE* f = fopen(file, "r");
    if (!f) {
        std::string e = fmt::format("Error while opening {}: {}", file, strerror(errno));
        throw shader_exception(e, errno, std::string{file});
    }

    char s[1024];
    while (!feof(f)) {
        memset(s, 0, 1024);
        fgets(s, 1023, f);

        std::string sview(s);

        auto words_begin =
            std::sregex_iterator(sview.begin(), sview.end(), regInclude);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string includeFile = match[1].str();

            std::string fullInclude{basePath};
            fullInclude.append("/");
            fullInclude.append(includeFile);

            sview = GLShader::readAndProcessFile(fullInclude.c_str());
            break;
        }

        data.append(sview);
    }

    return data;
}


std::string GLShader::readFile(const char* file)
{
    return this->readAndProcessFile(file);
}

void GLShader::compile()
{
    glCompileShader(this->_handle);

    GLint res = GL_FALSE;
    glGetShaderiv(this->_handle, GL_COMPILE_STATUS, &res);

    /* Retrieve shader logs anyway */
    GLint logsize = 0;
    glGetShaderiv(this->_handle, GL_INFO_LOG_LENGTH, &logsize);

    if (res == GL_TRUE && logsize > 1) {
        char* logdata = new char[logsize];

        glGetShaderInfoLog(this->_handle, logsize, NULL, logdata);
        std::string e = fmt::format("Error while compiling {}: {}", this->_file, logdata);
        throw shader_exception(e, -1023, std::string{_file});
    }
}

GLShaderProgram::GLShaderProgram(std::string_view name, const std::vector<GLShader*>& shaders)
    : ShaderProgram(name)
{
    if (shaders.size() > 0)
        this->_handle = glCreateProgram();

    for (auto s : shaders) {        
        _files.push_back(std::make_pair(s->getType(), s));
        glAttachShader(_handle, s->getHandle());
    }
}

void GLShaderProgram::link()
{
    glLinkProgram(this->_handle);

    GLint res = GL_FALSE;
    glGetProgramiv(this->_handle, GL_LINK_STATUS, &res);

    /* Retrieve shader logs anyway */
    GLint logsize = 0;
    glGetProgramiv(this->_handle, GL_INFO_LOG_LENGTH, &logsize);

    if (logsize > 1) {
        char* logdata = new char[logsize];

        glGetProgramInfoLog(this->_handle, logsize, NULL, logdata);
        std::string e = fmt::format("Error while compiling shader program '{}': {}", _name, logdata);

        // if (res == GL_TRUE)
        throw shader_exception(e, 1023, std::string{_name});
    }

    GFXService::getShaderManager()->addShader(this);
}

/**
 * Gets the uniform location on cache, or directly from the shader if not there
 *
 * We have a cache because getting this info from the shader is expensive, becayse, well
 * you need to talk to the video card, and even if the video card is fast, the transport is
 * slow, because of the PCI bus.
 * Even if you are using an APU, because APUs use the PCI bus to communicate with the processor
 */
GLint GLShaderProgram::getUniformLocation(std::string_view name)
{
    std::string sname{name};
    if (_uniform_cache.find(sname) == _uniform_cache.end()) {
        // Not found. Adds to the cache
        auto uniformVal = glGetUniformLocation(this->_handle, sname.c_str());

        if (uniformVal >= 0)
            _uniform_cache[sname] = uniformVal;

        return uniformVal;
    }

    return _uniform_cache[sname];
}

void GLShaderProgram::setUniform(std::string_view name, glm::vec3 val)
{
    glUniform3fv(this->getUniformLocation(name), 1, (const GLfloat*)&val[0]);
}

void GLShaderProgram::setUniform(std::string_view name, glm::vec4 val)
{
    glUniform4fv(this->getUniformLocation(name), 1, (const GLfloat*)&val[0]);
}

void GLShaderProgram::setUniform(std::string_view name, glm::mat4 val)
{
    glUniformMatrix4fv(this->getUniformLocation(name), 1, GL_FALSE, (const GLfloat*)&val[0][0]);
}

void GLShaderProgram::setUniform(std::string_view name, int val)
{
    glUniform1i(this->getUniformLocation(name), val);
}

void GLShaderProgram::setUniform(std::string_view name, float val)
{
    glUniform1f(this->getUniformLocation(name), val);
}

void GLShaderProgram::use()
{
    glUseProgram(this->_handle);
}

#endif
