#pragma once
/**
 * Those classes below mocks a renderer
 *
 * They are useful to test some aspects of the graphical system without
 * calling any specific API
 */

#include <client/graphical/camera.hpp>
#include <client/graphical/light.hpp>
#include <client/graphical/renderer.hpp>
#include <client/graphical/terrain_renderer.hpp>
#include <unordered_map>

class TestRenderer;

struct TestVertexHandle final : public familyline::graphics::VertexHandle {
private:
    TestRenderer& renderer_;

public:
    familyline::graphics::VertexData vdata;
    int id_ = 0;

    TestVertexHandle(
        familyline::graphics::VertexInfo& vinfo, familyline::graphics::VertexData vdata, int id,
        TestRenderer& renderer)
        : VertexHandle(vinfo), vdata(vdata), id_(id), renderer_(renderer)
    {
    }

    virtual bool update(familyline::graphics::VertexData& vd);
    virtual bool remove();
    virtual bool recreate(
        familyline::graphics::VertexData& vd, familyline::graphics::VertexInfo& vi);

    virtual ~TestVertexHandle() {}
};

class TestTerrainRenderer : public familyline::graphics::TerrainRenderer
{
public:    
    TestTerrainRenderer(familyline::graphics::Camera& cam) : TerrainRenderer(cam) {}

    virtual void buildTextures() {}

    virtual void buildVertexData() {}

    virtual void render(familyline::graphics::Renderer& rnd) {}

    virtual ~TestTerrainRenderer() {}
};

class TestRenderer : public familyline::graphics::Renderer
{
private:
    std::unordered_map<int, TestVertexHandle> handles_;
    int lastid_ = 0;
    std::vector<std::unique_ptr<familyline::graphics::LightHandle>> vlight_list_;

    TestTerrainRenderer* ttr = nullptr;
public:
    virtual familyline::graphics::VertexHandle* createVertex(
        familyline::graphics::VertexData& vd, familyline::graphics::VertexInfo& vi);

    virtual void removeVertex(familyline::graphics::VertexHandle* vh);
    virtual void render(familyline::graphics::Camera* c);

    virtual familyline::graphics::LightHandle* createLight(familyline::graphics::Light& light)
    {
        auto lhandle = std::make_unique<familyline::graphics::LightHandle>(light);
        auto ret     = lhandle.get();
        vlight_list_.push_back(std::move(lhandle));

        return ret;
    }

    virtual void removeLight(familyline::graphics::LightHandle* lh) {}

    virtual familyline::graphics::TerrainRenderer* createTerrainRenderer(
        familyline::graphics::Camera& camera)
    {
        if (!ttr)
            ttr = new TestTerrainRenderer{camera};
        
        return ttr;
    }

    size_t getVertexListCount();
    virtual ~TestRenderer() {
        if (ttr)
            delete ttr;
    }
};
