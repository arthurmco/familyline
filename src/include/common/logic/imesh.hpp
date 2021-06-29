#pragma once

#include <glm/glm.hpp>

/**
 * An "interface" to a mesh, something that represents a game object
 * in the screen
 */
namespace familyline::logic
{
class IMesh
{
public:
    virtual glm::vec3 getPosition() const    = 0;
    virtual void setLogicPosition(glm::vec3) = 0;
    virtual void addLogicPosition(glm::vec3) = 0;

    virtual ~IMesh() {}
};

}  // namespace familyline::logic
