#include <common/logic/ObjectPathManager.hpp>
//#include "DebugPlot.hpp"

#include <algorithm>

int maxpathID = 0;
using namespace familyline::logic;

static std::vector<glm::vec3> ConvertTo3DPath(std::vector<glm::vec2>* path)
{
    std::vector<glm::vec3> v3;
    for (const auto v : *path) {
        v3.push_back(glm::vec3(v.x, 512, v.y));
    }
    return v3;
}

/*  Adds a path from object 'o' to the path manager
    Returns true if added successfully, or false if there's already a path
    there for the same object
*/
bool ObjectPathManager::AddPath(GameObject* o, std::vector<glm::vec2> path)
{
    for (auto& ref : _pathrefs) {
        if (ref.lc->getID() == o->getID()) {
            /* If we had some object following a path and set a new
               path for that object, interrupt the path for this one
               and make it follow the new path

               This is the default behavior of all RTS games
            */
            ref.interrupted = true;
        }
    }

    std::remove_if(
        _pathrefs.begin(), _pathrefs.end(), [](ObjectPathRef& r) { return r.interrupted; });

    std::vector<glm::vec2>* ppath = new std::vector<glm::vec2>(path);

    _pathrefs.emplace_back(maxpathID++, o, ppath, 0);
    return true;
}

/*  Removes a path from object 'oid'.
    Returns true if path removed, or false if path didn't exist there */
bool ObjectPathManager::RemovePath(long oid)
{
    for (auto it = _pathrefs.begin(); it != _pathrefs.end(); it++) {
        if (it->lc->getID() == oid) {
            //          DebugPlotter::pinterface->RemovePath(it->dbg_path_plot);
            _pathrefs.erase(it);
            return true;
        }
    }

    return false;
}

/* Update the paths
 * Also, removes the completed paths
 * */
void ObjectPathManager::UpdatePaths(unsigned ms_frame)
{
    /* Store a vector of 'completed path' iterators.
     * Nuke them in iteration end */
    std::vector<int> compl_pathids;

    if (!_terr)
        return;

    assert(_terr);
    for (auto it = _pathrefs.begin(); it != _pathrefs.end(); it++) {
        if (it->path_point == it->path->back()) {
            /* Path completed. Remove the iterator */
            compl_pathids.push_back(it->pathid);
        }

        auto px = it->path_point.x;
        auto pz = it->path_point.y;

        it->current_time += ms_frame;

        auto lcpos = it->lc->getPosition();
        lcpos.x    = (px);
        lcpos.y    = (_terr->getHeightFromCoords(glm::vec2(px, pz)));
        lcpos.z    = (pz);
        it->lc->setPosition(lcpos);

        // 1 step = 0.1 second
        if (it->path_ptr < it->path->size() - 1) {
            const unsigned timedelta = it->current_time - it->last_step_time;

            unsigned cptr = it->path_ptr;
            for (unsigned i = 0; i < timedelta; i += 100) {
                if ((cptr + 1) < it->path->size()) cptr++;
            }

            it->path_ptr       = cptr;
            it->path_point     = (*it->path)[cptr];
            it->last_step_time = it->current_time;
        }
    }

    /* Delete the reserved iterators */
    for (auto& pathid : compl_pathids) {
        //      DebugPlotter::pinterface->RemovePath(it->dbg_path_plot);
        _pathrefs.erase(std::remove_if(
            _pathrefs.begin(), _pathrefs.end(),
            [&](const ObjectPathRef& ref) { return (ref.pathid == pathid); }));
    }
}

void ObjectPathManager::SetTerrain(Terrain* t) { _terr = t; }
