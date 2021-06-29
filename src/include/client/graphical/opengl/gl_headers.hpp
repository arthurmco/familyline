#pragma once

/**
 * Contains all opengl headers Familyline will need.
 * Also an include. If those headers were not available, OpenGL support will not be there.
 * (Renderers will be optional, but you will need to choose one.)
 *
 */

#if __has_include(<SDL2/SDL_opengl.h>) && __has_include(<GL/glew.h>)
#define RENDERER_OPENGL

#if !(defined(__gl_h_) || defined(__GL_H__) || defined(_GL_H) || defined(__X_GL_H))
#include <GL/glew.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#ifdef __linux__
#include <GL/glxew.h>
#endif

#else
#undef RENDERER_OPENGL
#endif


