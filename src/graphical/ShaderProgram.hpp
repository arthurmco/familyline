/***
    Shader program abstraction

    Copyright (C) 2016 Arthur M

***/

#include "Shader.hpp"
#include <glm/glm.hpp>

#include <map>

#ifndef SHADERPROGRAM_HPP
#define SHADERPROGRAM_HPP

namespace Tribalia {
namespace Graphics {

    class ShaderProgram
    {
    private:
        Shader* _vertex;
        Shader* _pixel;

        GLint _id;

		std::map<std::string, GLint> uniform_cache;

		/* Tries to get the uniform location.
			First, it query the cache, then it asks for OpenGL*/
		GLint GetUniformLocation(const char* name);
    public:
        ShaderProgram(Shader* vert, Shader* pixel);
        bool Link();
        void Use();

        /* Sets the uniform 'name' to 'value' on shader.
            Returns true if it could find the var, false if it couldn't*/
        bool SetUniform(const char* name, glm::mat4 value);
        bool SetUniform(const char* name, glm::vec3 value);
		bool SetUniform(const char* name, float value);
		bool SetUniform(const char* name, int value);

		bool SetUniformArray(const char* name, int count, float* value);
		bool SetUniformArray(const char* name, int count, glm::vec3* value);

        GLint GetID();

    };

}
}


#endif /* end of include guard: SHADERPROGRAM_HPP */
