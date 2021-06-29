#pragma once

/**
 * Camera representation in logic mode
 *
 * Since some player events deal with camera movementation and
 * positioning, also validation of camera position and look to see
 * if the object location is valid, we need to have a basic,
 * barebones camera representation in logic code
 *
 * Client and server might subclass it as well
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <glm/glm.hpp>

namespace familyline::logic
{
class ICamera
{
public:
    ICamera() {}

    virtual glm::vec3 GetPosition() const = 0;
    virtual void SetPosition(glm::vec3)   = 0;
    virtual void AddPosition(glm::vec3)   = 0;

    virtual glm::vec3 GetLookAt() const = 0;
    virtual void SetLookAt(glm::vec3)   = 0;
    virtual void AddLookAt(glm::vec3)   = 0;

    /*  Add rotation to the camera.
        You can rotate the camera by changing the look-at value in a
        'circular way'. I will use the glm rotation functions */
    virtual void AddRotation(glm::vec3 axis, float angle) = 0;
    
    virtual void AddZoomLevel(float) = 0;

    virtual ~ICamera() {}
};
}  // namespace familyline::logic
