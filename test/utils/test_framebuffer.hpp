#pragma once
/**
 * The classes below mocks a framebuffer
 *
 * They are useful to test some aspects of the graphical system without
 * calling any specific API
 */
#include <client/graphical/framebuffer.hpp>

class TestFramebuffer : public familyline::graphics::Framebuffer
{
public:
    TestFramebuffer(std::string_view name, int width, int height) : Framebuffer(name, width, height)
    {
    }

    /// Call this before you start drawing data
    virtual void startDraw() {}

    /// Call this after you end drawing data
    virtual void endDraw() {}

    virtual int getTextureHandle()
    {
        return int((uintptr_t(this) & 0xffffffff) + (uintptr_t(this) << 32));
    }

    virtual ~TestFramebuffer(){};
};
