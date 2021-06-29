#pragma once

#include <vector>

#include "vertexdata.hpp"

namespace familyline::graphics
{
typedef std::vector<struct VertexData> VertexDataGroup;

/**
 * \brief Base animator class
 *
 * This animation class is a wrapper to support animation in all meshes
 * \see StaticAnimator
 * \see DeformAnimator
 */
class Animator
{
protected:
    /**
     * The base vertex data, the one that the mesh file
     * shows in its vertex list
     */
    VertexDataGroup base_vdata;

    bool dirtyFrame = true;

public:
    /**
     * Get vertex data from the current frame
     */
    virtual VertexDataGroup getCurrentFrame() = 0;

    /**
     * Advance animation by 'ms' milisseconds
     *
     * If ms is not the frametime of this animation, interpolate
     */
    virtual void advance(double ms) = 0;

    /**
     * Run some predefined animation named 'name'
     *
     * Set the internal pointer to the first frame of that animation
     */
    virtual void runAnimation(const char* name) = 0;

    /**
     * Get the current time, in ms, of the current animation
     */
    virtual double getCurrentTime() { return 0.0; }

    virtual ~Animator() {}
    
    bool isDirty() { return this->dirtyFrame; }
};
}  // namespace familyline::graphics
