#pragma once

/**
 * Defines settings for illumination sources
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <variant>

namespace familyline::graphics
{
/**
 * The global light we have
 * It has only a direction (in the form of a normal), a power value
 * and a color value
 *
 * Only the first sun light you add will be used by the engine
 */
struct SunLightType {
    glm::vec3 direction;
};

/**
 * Point light
 *
 * It has a position, a power value and a color value
 *
 * WARNING: not implemented
 */
struct PointLightType {
    glm::vec3 position;
};

/**
 * The actual light type
 *
 * A true type, that differs between values, not a weak union.
 */
using LightType = std::variant<SunLightType, PointLightType>;

class Light
{
private:
    LightType type_;
    float power_;
    glm::vec3 color_;
    std::string name_;

public:
    glm::vec3 direction;

    Light(LightType type, float power, glm::vec3 color, std::string name)
        : type_(type), power_(power), color_(color), name_(name)
    {
    }

    LightType& getType() { return type_; }

    float getPower() const { return power_; }
    glm::vec3 getColor() const { return color_; }
    std::string_view getName() const { return name_; }
};

}  // namespace familyline::graphics
