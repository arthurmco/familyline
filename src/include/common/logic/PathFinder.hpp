/***
    Path finder class.

    Copyright (C) 2016-2018, 2020 Arthur M

***/
#pragma once

#include <array>
#include <common/logic/terrain.hpp>
#include <glm/glm.hpp>
#include <list>
#include <queue>
#include <unordered_map>
#include <vector>

#include "game_event.hpp"
#include "object_components.hpp"
#include "object_manager.hpp"

namespace familyline::logic
{
class PathFinder;

/**
 * Path node
 */
struct PathNode {
    PathFinder* const pf;
    glm::vec2 pos;             ///< Position
    PathNode* prev = nullptr;  ///< The previous nodes in the node list

    double f = 0;

    bool visited;

    PathNode(PathFinder* const pf, glm::vec2 pos);

    /**
     * Get all possible neighbors for a node
     */
    std::vector<PathNode*> getNeighbors();

    /**
     * Calculate the heuristic
     */
    double getHeuristic(glm::vec2 end, glm::vec2 start);
};

/**
 * Path node list definitions
 */
struct NodeList {
    std::list<PathNode*> nodes;
    PathFinder* const pf;
    unsigned char* pathlist;
    unsigned width;

    NodeList(PathFinder* const pf, unsigned char* pathlist = nullptr, unsigned w = 0);

    /***
     * Check if the object can fit in the specified position
     *
     * Even if 'pos' is emply, maybe the object that is on pos, whose size
     * is specified by objw x objh, will not fit
     */
    bool canObjectFit(glm::vec2 pos, double objw, double objh);

    /**
     * Check if node has an obstacle on it
     */
    bool isObstacle(glm::vec2 pos);

    /**
     * Get a node from the node definitions
     * Makes easier to create a node and ensure that it have the same properties
     * (score, etc) for the same paths
     */
    PathNode* getNode(glm::vec2 pos);
};

class PathFinder
{
    friend struct PathNode;

private:
    ObjectManager* _om;
    NodeList* node_list = nullptr;
    ;

    /**
     * The object event listener
     */
    ObjectEventReceiver oel;

    /**
     * The list, where we keep the objects
     */
    std::vector<const GameObject*> objList;

    /**
     * \brief Pathfinder slots
     *
     * Might be reused as a bitmask
     */
    unsigned char* _pathing_slots = nullptr;
    int _mapWidth, _mapHeight;

    /**
     * \brief Converts a linked list of pathnodes in a list of paths.
     *
     * Gets the last node and rebuild the path from there, then revert the path.
     */
    std::vector<glm::vec2> BuildPath(PathNode* last);

    std::vector<glm::vec2> FindPath(glm::vec2 start, glm::vec2 end, double width, double height);

public:
    PathFinder(ObjectManager*);
    ~PathFinder();

    void InitPathmap(int w, int h);
    void UpdatePathmap(int w, int h, int x = 0, int y = 0);
    void ClearPathmap(int w, int h, int x, int y);

    std::vector<glm::vec2> CreatePath(const GameObject& o, glm::vec2 destination);
};

}  // namespace familyline::logic
