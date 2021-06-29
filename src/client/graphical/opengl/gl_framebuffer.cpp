#include <client/graphical/exceptions.hpp>
#include <client/graphical/opengl/gl_framebuffer.hpp>
#include <common/logger.hpp>

#ifdef RENDERER_OPENGL

using namespace familyline::graphics;

GLFramebuffer::GLFramebuffer(std::string_view name, int width, int height) :
    Framebuffer(name, width, height)
{
    auto& log = LoggerService::getLogger();
    glGetError();

    glGenFramebuffers(1, &_handle);
    glBindFramebuffer(GL_FRAMEBUFFER, _handle);

    log->write(
        "fb", LogType::Debug, "creating framebuffer %s: handle=%#x, size=%d x %d", name.data(),
        _handle, width, height);

    glGenRenderbuffers(1, &_rboHandle);
    glBindRenderbuffer(GL_RENDERBUFFER, _rboHandle);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _rboHandle);

    this->setupTexture(width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _textureHandle, 0);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        char e[128];
        snprintf(e, 127, "error %#x while setting texture for framebuffer %s", err, name.data());
        throw graphical_exception(std::string(e));
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        log->write(
            "fb", LogType::Warning, "framebuffer %s (%#x) is not complete", name.data(),
            _handle);  // should be error
    }

    err = glGetError();
    if (err != GL_NO_ERROR) {
        char e[128];
        snprintf(e, 127, "error %#x while creating framebuffer %s", err, name.data());
        throw graphical_exception(std::string(e));
    }

    log->write(
        "fb", LogType::Debug, "\tcompleted: texhandle=%#x, rbohandle=%#x", _textureHandle,
        _rboHandle);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/// Setup the texture bound to the framebuffer
///
/// Each framebuffer has a texture attached to it. All rendering commands sent to the framebuffer
/// will write to the texture, and the texture will have the final rendered 2d image
void GLFramebuffer::setupTexture(int width, int height)
{
    glGenTextures(1, &_textureHandle);
    glBindTexture(GL_TEXTURE_2D, _textureHandle);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        char e[128];
        snprintf(e, 127, "error %#x while creating or binding framebuffer texture", err);
        throw graphical_exception(std::string(e));
    }

    // Allocate an empty buffer to use as the framebuffer texture.
    // Other operating systems do not need this, but MacOS seems to need
    // or the framebuffer will preserve the content from the last run,
    // leading to bugs such as this: https://imgur.com/3HpjD7W
    char* c = new char[width*height*4];
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, c);
    err = glGetError();
    if (err != GL_NO_ERROR) {
        delete[] c;
        char e[128];
        snprintf(e, 127, "error %#x while setting up framebuffer texture", err);
        throw graphical_exception(std::string(e));
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    delete[] c;
}

void GLFramebuffer::startDraw()
{
    glBindRenderbuffer(GL_RENDERBUFFER, _rboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, _handle);
}

void GLFramebuffer::endDraw()
{
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int GLFramebuffer::getTextureHandle() { return _textureHandle; }

GLFramebuffer::~GLFramebuffer() {
    glDeleteFramebuffers(1, &_handle);
    glDeleteTextures(1, &_textureHandle);
}

#endif
