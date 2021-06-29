#pragma once

/**
 * Draws the preview of the next building you will
 * build.
 *
 * It needs to be somehow connected to the renderer, because we will use a shader to set the
 * color of the building.
 */

#include <client/graphical/renderer.hpp>
#include <client/graphical/mesh.hpp>
#include <client/input/input_service.hpp>
#include <client/input/InputPicker.hpp>
#include <memory>
#include <optional>

namespace familyline
{
namespace logic
{
class ObjectFactory;
}

/**
 * Previews an entity (preferentially a building) that will be
 * created.
 *
 * We only store the currently previewed entity
 *
 * Since this class will be mostly used to preview buildings, the functions
 * will use buildings as examples.
 */
class PreviewRenderer
{
public:
    PreviewRenderer(logic::ObjectFactory&, graphics::Renderer&, input::InputPicker& ip);

    ~PreviewRenderer();

    /// Set the previewed building to the one of type `type`.
    ///
    /// Returns true if the entity exists and is visible, false if neither
    /// exists nor is visible.
    ///
    /// Also sets if we can build the object. We pass this here, because it is
    /// only a renderer, not a class that will detect if the building can be
    /// placed.
    bool add(std::string type, bool possible);

    /// Reset the preview, disabling it
    void reset();

private:

    struct PreviewInfo {
        std::shared_ptr<graphics::Mesh> m;
        std::vector<graphics::VertexHandle*> vhandles;
    };
    
    logic::ObjectFactory& of_;
    graphics::Renderer& rndr_;
    input::InputPicker& ip_;
        
    input::listener_handler_t lh_;
    long long int rndr_hook_;

    /// Is preview enabled or not?
    bool enable_ = false;

    /// The data of the previewed building, can be no one if no object
    /// will be built.
    std::optional<std::tuple<PreviewInfo, bool /* possible */>> prev_data_;

    /// Last previewed coordinates.
    std::optional<glm::vec3> coords_;

    /// Private, get the mouse coordinates (they will come from a callback) and
    /// plot them.
    glm::vec3 getMouseCoords(glm::vec2 pos) const;
};

};  // namespace familyline
