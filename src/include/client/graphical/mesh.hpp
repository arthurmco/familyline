#pragma once

#include <common/logic/imesh.hpp>
#include <glm/fwd.hpp>
#include <string>
#include <optional>

#include "animator.hpp"
#include "asset_object.hpp"
#include <queue>

namespace familyline::graphics
{
struct BoundingBox {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
};

class Mesh : public AssetObject, public familyline::logic::IMesh
{
private:
    std::string _name;
    Animator* _ani;
    glm::mat4 _worldMatrix;
    glm::vec3 worldPosition;
    std::vector<VertexInfo> vinfo;

public:
    Mesh(const char* name, Animator* ani, std::vector<VertexInfo> vinfo)
        :  _name(name), _ani(ani), _worldMatrix(glm::mat4(1.0))
    {
        this->vinfo = vinfo;
    }

    virtual ~Mesh() {
        delete _ani;
    }
    
    /**
     * Update the world matrix and the animator
     */
    virtual void update();

    virtual void setLogicPosition(glm::vec3 p) { this->worldPosition = p; }
    virtual void addLogicPosition(glm::vec3 p) { this->worldPosition += p; }

    virtual std::string_view getName() const;
    virtual glm::vec3 getPosition() const;
    virtual glm::mat4 getWorldMatrix() const;

    virtual std::vector<VertexInfo> getVertexInfo() const { return vinfo; }
    virtual void setVertexInfo(std::vector<VertexInfo>&& v) { vinfo = v; }

    virtual std::vector<VertexData> getVertexData();
    virtual bool isVertexDataDirty();

    // TODO: create a better function to return bounding boxes
    BoundingBox getBoundingBox();

    Animator* getAnimator();

    virtual AssetType getAssetType() const { return AssetType::MeshAsset; }
};
}  // namespace familyline::graphics
