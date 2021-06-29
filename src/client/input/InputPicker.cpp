#include <algorithm>
#include <client/input/InputPicker.hpp>
#include <common/logic/logic_service.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace familyline::input;
using namespace familyline::graphics;
using namespace familyline::logic;

InputPicker::InputPicker(
    Terrain* terrain, Window* win, SceneManager* sm, Camera* cam, ObjectManager* om)
{
    this->_terrain = terrain;
    this->_win     = win;
    this->_sm      = sm;
    this->_cam     = cam;
    this->_om      = om;

    //    ObjectEventEmitter::addListener(&oel);
}

/* Get cursor ray in screen space */
glm::vec4 InputPicker::GetCursorScreenRay()
{
    int w = 0, h = 0, x = 0, y = 0;
    _win->getSize(w, h);
    Cursor::GetInstance()->GetPositions(x, y);

    // Create ray data.
    glm::vec4 ray = glm::vec4((2.0f * x) / w - 1.0f, 1.0f - (2.0f - y) / h, -1.0f, 1.0f);
    return ray;
}

/* Get cursor ray in eye space */
glm::vec4 InputPicker::GetCursorEyeRay()
{
    glm::vec4 cur_eye = glm::inverse(_cam->GetProjectionMatrix()) * this->GetCursorScreenRay();
    return glm::vec4(cur_eye.x, cur_eye.y, -1.0f, 0.0f);
}

/* Get cursor ray in world space */
glm::vec3 InputPicker::GetCursorWorldRay()
{
    /*
      glm::vec4 cur_world = glm::inverse(_cam->GetViewMatrix()) * this->GetCursorEyeRay();
      glm::vec3 cur_world3 = glm::vec3(cur_world.x, cur_world.y, cur_world.z);
      return glm::normalize(cur_world3);
    */
    int x = 0, y = 0, w = 0, h = 0;
    _win->getSize(w, h);
    Cursor::GetInstance()->GetPositions(x, y);

    glm::vec3 cStart = glm::unProject(
        glm::vec3(x, h - y, 0), _cam->GetViewMatrix(), _cam->GetProjectionMatrix(),
        glm::vec4(0, 0, w, h));
    glm::vec3 cEnd = glm::unProject(
        glm::vec3(x, h - y, 1), _cam->GetViewMatrix(), _cam->GetProjectionMatrix(),
        glm::vec4(0, 0, w, h));
    glm::vec3 cur_world = glm::normalize(cEnd - cStart);
    return cur_world;
}

/* Check if we intersect with the terrain between between start and end */
bool InputPicker::CheckIfTerrainIntersect(glm::vec3 ray, float start, float end)
{
    glm::vec3 pStart = _cam->GetPosition() + (ray * start);
    glm::vec3 pEnd   = _cam->GetPosition() + (ray * end);

    if (pStart.y >= 0 && pEnd.y < 0) {
        return true;
    }

    return false;
}

void InputPicker::UpdateTerrainProjectedPosition()
{
    glm::vec3 cur_world = this->GetCursorWorldRay();

    /*printf("\ncamera_pos: %.4f %.4f %.4f, ray_pos: %.4f %.4f %.4f\n",
      _cam->GetPosition().x, _cam->GetPosition().y, _cam->GetPosition().z,
      cur_world.x, cur_world.y, cur_world.z);
    */
    float prolong_near = 0.1f, prolong_far = 128.0f;
    float prolong_now = prolong_near + ((prolong_far - prolong_near) / 2.0f);

    glm::vec3 pHalf = glm::vec3(64, 0, 64);
    auto [width, height] = _terrain->getSize();

    //  printf("near: %.3f %.3f %.3f, far: %.3f %.3f %.3f, prolongs: { ",
    //         pNear.x, pNear.y, pNear.z, pFar.x, pFar.y, pFar.z);
    for (int i = 0; i < MAX_PICK_ITERATIONS; i++) {
        /*  Here, we'll check if the ray projection is above or below the terrain

            If we're above, we'll adjust pNear to that point
            If we're below, we'll adjust pFar to that point

            To check that, we simply check if pFar is under and
            pNear and pHalf are above
        */

        if (this->CheckIfTerrainIntersect(cur_world, prolong_near, prolong_now))
            prolong_far = prolong_now;
        else
            prolong_near = prolong_now;

        prolong_now = prolong_near + ((prolong_far - prolong_near) / 2.0f);
        pHalf       = _cam->GetPosition() + (cur_world * prolong_now);
        // printf("%.2f (%.2f %.2f %.2f), ", prolong_now, pHalf.x, pHalf.y, pHalf.z);
    }

    glm::vec3 collide = _terrain->graphicalToGame(pHalf);

    /* Clamp collide to the terrain area */
    if (collide.x >= width)
        collide.x = width - 1;

    if (collide.z >= height)
        collide.z = height - 1;

    if (collide.x < 0) collide.x = 0;

    if (collide.z < 0) collide.z = 0;

    auto pointHeight = _terrain->getHeightFromCoords(glm::vec2(collide.x, collide.z));
    if (collide.x > 0 && collide.z > 0)
        collide.y = pointHeight;
    // printf(" }\nprol: %.2f, pos: %.3f %.3f %.3f, gamespace: %.3f %.3f %.3f\n\n",
    //  1.0f, pHalf.x, pHalf.y, pHalf.z, collide.x, collide.y, collide.z);

    _intersectedPosition = collide;
}

void InputPicker::UpdateIntersectedObject()
{
    glm::vec3 direction = this->GetCursorWorldRay();
    glm::vec3 origin = _cam->GetPosition();

    const auto& olist = familyline::logic::LogicService::getObjectListener();

    std::set<object_id_t> toAdd;
    std::set<object_id_t> toRemove;

    std::map<object_id_t, bool> aliveMap;
    auto aliveObjects = olist->getAliveObjects();
    for (auto obj : this->poi_list) {
        // Stored object is still alive
        if (aliveObjects.find(obj.ID) != aliveObjects.end()) {
            aliveMap[obj.ID] = true;
        }

        // Stored object is not alive anymore
        if (aliveObjects.find(obj.ID) == aliveObjects.end()) {
            aliveMap[obj.ID] = false;
            toRemove.insert(obj.ID);
        }
    }

    for (auto objid : aliveObjects) {
        // We have a new stored object
        if (aliveMap.find(objid) == aliveMap.end()) {
            toAdd.insert(objid);
        }
    }

    for (auto objid : toRemove) {
        poi_list.erase(std::remove_if(
            poi_list.begin(), poi_list.end(),
            [&](const PickerObjectInfo& poi) { return poi.ID == objid; }));
    }

    for (auto objid : toAdd) {
        auto object = _om->get(objid).value();

        if (!object->getLocationComponent()) {
            continue;
        }

        auto mesh = object->getLocationComponent()->mesh;
        poi_list.emplace_back(object->getPosition(), std::dynamic_pointer_cast<Mesh>(mesh), objid);
    }

    // Check the existing objects
    for (const PickerObjectInfo& poi : poi_list) {
        auto obj = _om->get(poi.ID);
        if (!obj)
            continue;

        auto osize = (*obj)->getSize();
        auto gsize = _terrain->gameToGraphical(
            glm::vec3(osize.x, 0, osize.y));

        if (!poi.mesh)
            continue;

        auto gpos = poi.mesh->getPosition();

        glm::vec4 vmin = glm::vec4(-gsize.x/2, gpos.y, -gsize.z/2, 1);
        glm::vec4 vmax = glm::vec4(gsize.x/2, glm::max(gsize.x, gsize.z),
            gsize.z/2, 1);

        vmin = poi.mesh->getWorldMatrix() * vmin;
        vmax = poi.mesh->getWorldMatrix() * vmax;

        glm::min(vmin.y, 0.0f);
        glm::max(vmax.y, 0.0f);

        // glm::vec3 planePosX, planePosY, planePosZ;

        float tmin = -100000;
        float tmax = 100000;

        float dxmin, dxMax;
        if (direction.x != 0) {
            dxmin = (vmin.x - origin.x) / direction.x;
            dxMax = (vmax.x - origin.x) / direction.x;

            tmin = fmaxf(tmin, fminf(dxmin, dxMax));
            tmax = fminf(tmax, fmaxf(dxmin, dxMax));
            // printf("x: %.4f %.4f \t", dxmin, dxMax);
            if (tmax < tmin) continue;
        }

        if (direction.y != 0) {
            dxmin = (vmin.y - origin.y) / direction.y;
            dxMax = (vmax.y - origin.y) / direction.y;

            tmin = fmaxf(tmin, fminf(dxmin, dxMax));
            tmax = fminf(tmax, fmaxf(dxmin, dxMax));
            // printf("y: %.4f %.4f \t", dxmin, dxMax);
            if (tmax < tmin) continue;
        }

        if (direction.z != 0) {
            dxmin = (vmin.z - origin.z) / direction.z;
            dxMax = (vmax.z - origin.z) / direction.z;

            tmin = fmaxf(tmin, fminf(dxmin, dxMax));
            tmax = fminf(tmax, fmaxf(dxmin, dxMax));
            // printf("z: %.4f %.4f \t", dxmin, dxMax);
            if (tmax < tmin) continue;
        }

        // printf("total: %.4f %.4f\n", tmin, tmax);

        /* Ray misses */
        if (tmin < 0) {
            continue;
        }

        /* Collided with both 3 axis! */
        if (tmax >= tmin) {
            auto lobj = _om->get(poi.ID);
            if (!lobj.has_value()) return;

            _locatableObject = lobj.value();

            if (!_locatableObject.expired()) return;
        }
    }

    _locatableObject = std::weak_ptr<GameObject>();
}

/*  Get position where the cursor collides with the
    terrain, in render coordinates */
glm::vec3 InputPicker::GetTerrainProjectedPosition()
{
    return _terrain->gameToGraphical(_intersectedPosition);
}

/*  Get position where the cursor collides with the
    terrain, in game coordinates */
glm::vec2 InputPicker::GetGameProjectedPosition()
{
    glm::vec3 intGame = _intersectedPosition;
    return glm::vec2(intGame.x, intGame.z);
}

/*  Get the object that were intersected by the cursor ray */
std::weak_ptr<GameObject> InputPicker::GetIntersectedObject() { return this->_locatableObject; }
