#include "test_renderer.hpp"

using namespace familyline::graphics;

VertexHandle* TestRenderer::createVertex(VertexData& vd, VertexInfo& vi)
{
    int id = ++lastid_;

    handles_.emplace(std::make_pair(id, TestVertexHandle(vi, vd, id, *this)));
    auto& element = handles_.at(id);
    return dynamic_cast<VertexHandle*>(&element);
}

void TestRenderer::removeVertex(VertexHandle* vh)
{
    TestVertexHandle* tvh = dynamic_cast<TestVertexHandle*>(vh);
    if (!tvh) return;

    handles_.erase(tvh->id_);
}

void TestRenderer::render(Camera* c)
{
    // noop here?
}

size_t TestRenderer::getVertexListCount() { return handles_.size(); }

bool TestVertexHandle::update(VertexData& vd)
{
    vdata = vd;
    return true;
}

bool TestVertexHandle::remove()
{
    renderer_.removeVertex(this);
    return true;
}

bool TestVertexHandle::recreate(VertexData& vd, VertexInfo& vi)
{
    vinfo = vi;
    vdata = vd;
    return true;
}
