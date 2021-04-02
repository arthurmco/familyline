#include <algorithm>
#include <client/graphical/asset_object.hpp>
#include <client/graphical/mesh.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numeric>
#include <iterator>

using namespace familyline::graphics;

void Mesh::update()
{
    auto translMatrix = glm::mat4(1.0);
    translMatrix      = glm::translate(translMatrix, this->worldPosition);

    _worldMatrix = translMatrix;

    // update values in the shader state
    for (auto& vi : vinfo) {
        vi.shaderState.matrixUniforms["mWorld"] = _worldMatrix;
    }
}

std::string_view Mesh::getName() const { return _name; }

glm::vec3 Mesh::getPosition() const { return this->worldPosition; }

glm::mat4 Mesh::getWorldMatrix() const { return _worldMatrix; }

Animator* Mesh::getAnimator() { return _ani; }

std::vector<VertexData> Mesh::getVertexData() { return _ani->getCurrentFrame(); }

bool Mesh::isVertexDataDirty() { return _ani->isDirty(); }

// TODO: create a better function to return bounding boxes
BoundingBox Mesh::getBoundingBox()
{
    auto vdx = _ani->getCurrentFrame();

    std::vector<BoundingBox> binit;
    std::transform(vdx.begin(), vdx.end(), std::back_inserter(binit), [](VertexData vd) {
        BoundingBox b;
        b.minX = b.minY = b.minZ = 99999999.0f;
        b.maxX = b.maxY = b.maxZ = -99999999.0f;
        return std::accumulate(
            vd.position.begin(), vd.position.end(), b, [](BoundingBox bit, glm::vec3 v) {
                bit.maxX = glm::max(v.x, bit.maxX);
                bit.maxY = glm::max(v.y, bit.maxY);
                bit.maxZ = glm::max(v.z, bit.maxZ);
                bit.minX = glm::min(v.x, bit.minX);
                bit.minY = glm::min(v.y, bit.minY);
                bit.minZ = glm::min(v.z, bit.minZ);

                return bit;
            });
    });

    BoundingBox b;
    b.minX = b.minY = b.minZ = 99999999.0f;
    b.maxX = b.maxY = b.maxZ = -99999999.0f;
    return std::accumulate(binit.begin(), binit.end(), b, [](BoundingBox cur, BoundingBox prev) {
        cur.maxX = glm::max(prev.maxX, cur.maxX);
        cur.maxY = glm::max(prev.maxY, cur.maxY);
        cur.maxZ = glm::max(prev.maxZ, cur.maxZ);
        cur.minX = glm::min(prev.minX, cur.minX);
        cur.minY = glm::min(prev.minY, cur.minY);
        cur.minZ = glm::min(prev.minZ, cur.minZ);

        return cur;
    });
}
