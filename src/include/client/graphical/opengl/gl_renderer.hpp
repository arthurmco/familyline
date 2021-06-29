#pragma once

#include <vector>

#include <client/graphical/opengl/gl_headers.hpp>
#include <client/graphical/renderer.hpp>
#include <client/graphical/terrain_renderer.hpp>

#ifdef RENDERER_OPENGL

namespace familyline::graphics
{
class GLRenderer;

struct GLVertexHandle final : public VertexHandle {
private:
    GLRenderer& _renderer;

public:
    int vao;
    int vboPos, vboNorm, vboTex;
    size_t vsize;

    GLVertexHandle(int vao, GLRenderer& renderer, VertexInfo& vinfo)
        : VertexHandle(vinfo), vao(vao), _renderer(renderer)
    {
    }

    virtual bool update(VertexData& vd);
    virtual bool remove();
    virtual bool recreate(VertexData& vd, VertexInfo& vi);

    virtual ~GLVertexHandle(){};
};

class GLRenderer : public Renderer
{
private:
    std::vector<std::unique_ptr<GLVertexHandle>> _vhandle_list;

    std::vector<std::unique_ptr<LightHandle>> vlight_list_;

    /// Current-used directional light
    LightHandle* directionalLight_ = nullptr;

    /// Vertices scheduled to be removed
    /// Since we cannot remove them in the removeVertex(), because the
    /// removed one itself calls this method, we schedule them to be removed
    /// by the next call to render()
    std::vector<GLVertexHandle*> to_be_removed_handles_;

    void removeScheduledVertices();
    
    ShaderProgram* _sForward = nullptr;
    ShaderProgram* _sLines   = nullptr;

    std::unique_ptr<TerrainRenderer> terrain_renderer_;

public:
    GLRenderer();

    virtual VertexHandle* createVertex(VertexData& vd, VertexInfo& vi);
    virtual void render(Camera* c);

    /**
     * Create a raw VAO, with the vbox for position, normal and texture.
     *
     * Useful when we need to only retrieve the basic elements VAO, without an object
     */
    std::tuple<int, int, int, int> createRaw(VertexData& vd, ShaderProgram& shader);

    virtual void removeVertex(VertexHandle* vh);

    virtual LightHandle* createLight(Light& light);
    virtual void removeLight(LightHandle* lh);

    virtual TerrainRenderer* createTerrainRenderer(Camera& camera);

    
    /**
     * Set a shader to draw the available lights on
     */
    void drawLights(ShaderProgram& sp);
    
    virtual ~GLRenderer(){};
};
}  // namespace familyline::graphics

#endif
