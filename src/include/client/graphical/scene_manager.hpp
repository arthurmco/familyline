#pragma once

/**
 * Scene manager class
 *
 * Gets the graphical objects from different sources and checks
 * if they can be renderer or not
 *
 * Also run the animations for the meshes.
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <client/graphical/camera.hpp>
#include <client/graphical/light.hpp>
#include <client/graphical/renderer.hpp>
#include <client/graphical/scene_object.hpp>
#include <client/graphical/vertexdata.hpp>
#include <memory>

namespace familyline::graphics
{
struct SceneObjectInfo {
    std::shared_ptr<SceneObjectBase> object;

    int id;

    /// Is the object visible or not?
    bool visible;

    /// The vertex handles, represent data on the video card
    /// about this object
    std::vector<VertexHandle*> handles;

    /// The light handles, represent data about things on that
    /// object that emit light
    std::vector<LightHandle*> lights;

    SceneObjectInfo(
        std::shared_ptr<SceneObjectBase> o, int id, bool visible, std::vector<VertexHandle*> hs,
        std::vector<LightHandle*> ls)
        : object(std::move(o)), id(id), visible(visible), handles(hs), lights(ls)
    {
    }
};

typedef int scene_object_handle_t;

/**
 * Manages scene object rendering
 *
 * Gets scene object raw vertex and light handles and updates them, accordingly to
 * some variables.
 * For example, if the scene object is destroyed, we remove the vertex handle (and, therefore, the
 * data) from the video card
 */
class SceneManager
{
private:
    Camera& camera_;
    Renderer& renderer_;

    std::vector<SceneObjectInfo> objects_;

    /**
     *  Update the vertices of the visible objects into the
     * renderer and, consequently, on the video card.
     */
    void updateObjectVertices(SceneObjectInfo& soi);

    /**
     * Advance animation frames for all visible frames for the
     * specified scene obejct
     */
    void updateAnimations(SceneObjectInfo& soi, unsigned int ms);

    /// The default dark light
    Light defaultDark{SunLightType{glm::vec3(0, 0, 0)}, 0.0, glm::vec3(0, 0, 0), "defaultdark"};
    Light& directionalLight_;

public:
    SceneManager(Renderer& renderer, Camera& camera)
        : renderer_(renderer), camera_(camera), directionalLight_(defaultDark)
    {
    }

    /// Add the object to the scene manager, get the ID
    scene_object_handle_t add(std::shared_ptr<SceneObjectBase> o);
    void remove(scene_object_handle_t meshHandle);

    /// Update the scene objects
    void update(unsigned int ms);

    /// Get the primary directional light, the "sun/moon" light source of the scene
    ///
    /// If the directional light is not found, get a default light that is fully dark
    ///
    /// The shaders currently require a directional light source, so this function will always
    /// return a reference to a light object
    Light& getDirectionalLight() { return directionalLight_; }
};

}  // namespace familyline::graphics
