/**
 * Pathfinder implementation
 *
 * Copyright (C) 2021 Arthur Mendes
 */
#pragma once

#include <common/logic/terrain.hpp>
#include <glm/glm.hpp>
#include <list>
#include <memory>
#include <vector>

namespace familyline::logic
{
/**
 * Basic A* pathfinder implementation
 *
 * The A* algorithm works like this:
 *    - we operate on nodes. They are like the minimum unit of path plus some info
 *    - each node has two scores:
 *       - g: the cost of the path from the starting node to the current one
 *       - h: the estimated cost of the path from the current node to the goal
 *    - also, we have f, that is g+h
 *
 *    - for each node, we get all the possible neighbor nodes (the ones that are not blocked)
 *      - each neighbor node will have the current node as a parent
 *      - we will calculate f for each node
 *      - the node with the lower score wins
 *
 *    - Every node that won is placed on the closed list. Every node that we are calculating
 *      is placed on the open list
 *    - If one of our neighbors are already in the open list, we check if our g() is less than the
 *      one that is already there
 *      - if it is, then that one has a better path. Set the parent of that one to the current node
 *      - if not, set the parent of this one to the current node.
 *
 *    If we find the target node, then we stop and backtrace our path
 *    If the open list gets empty, then we do not have a path. A fallback might be retrying, or
 * backtracing from the closest one (the one with the lower f()? )
 *
 *    (Check https://medium.com/@nicholas.w.swift/easy-a-star-pathfinding-7e6689c7f7b2 if my
 *     explanation is too hard)
 *
 *
 *   Here, we also limit the number of iterations for each pathing operation, to not calculate a
 * huge path once per frame. Do not worry, if you do not reach the destination, the point will
 * actually be the point that is closer to there.
 */
class Pathfinder
{
public:
    Pathfinder(const Terrain& t) : t_(t), obstacle_bitmap_(1 * 1, false) {}

    /**
     * Update the obstacle bitmap, and the obstacle bitmap size ratio, compared to the
     * terrain size
     */
    void update(std::vector<bool>, int ratio = 1);

    struct TerrainTile {
        unsigned int height;
        TerrainType& type;
    };

    struct PathNode {
        glm::vec2 position;
        double height;

        double g;
        double h;
        PathNode* parent = nullptr;

        double f() const { return g + h; }

        void calculateG(glm::vec2 start, glm::vec2 end, const TerrainTile& current);
        void calculateValues(glm::vec2 start, glm::vec2 end, const TerrainTile& current);

        PathNode(glm::vec2 pos) : position(pos), height(0.0), g(0.0), h(0.0) {}
    };

    /**
     * Find a path through the terrain
     *
     * Return a vector of X+Z position (the height is guessed), ordered, from start to finish
     */
    std::vector<glm::vec2> findPath(
        glm::vec2 start, glm::vec2 end, glm::vec2 size, int maxiters = 200);

    bool maxIterReached() const { return has_max_iter_reached_; }

    bool hasPossiblePath() const {
        if (open_list_.size() == 0)
            return closed_list_.size() == 0;

        return true;
    }

private:
    const Terrain& t_;
    std::vector<bool> obstacle_bitmap_;

    std::list<std::unique_ptr<PathNode>> open_list_;
    std::list<std::unique_ptr<PathNode>> closed_list_;

    /**
     * Ratio of the obstacle bitmap
     *
     * For example, if this value is 2, the bitmap width has half of the size
     * of the terrain width, and the bitmap height is half of the size of the terrain
     * height
     */
    int ratio_ = 1;
    
    bool has_max_iter_reached_ = false;

    /**
     * Traverse the path, from begin to end, putting the possible nodes in the open_list and the
     * closed nodes in the closed list
     *
     * Returns the node of the end path, or nullptr if no path is possible
     * (TODO: fix that, make it always return a path!)
     *
     * Please do not clean the closed_list_ before using the PathNode* we return, or you will
     * crash the program.
     */
    PathNode* traversePath(glm::vec2 start, glm::vec2 end, glm::vec2 size, int maxiters);

    /**
     * Calculate the path positions
     *
     * The first element is the position of the last child, (i.e, the end), and the first element is
     * the position of the first one, i.e, the start
     *
     * If the obstacle bitmap ratio is more than 1, interpolate the points
     */
    std::vector<glm::vec2> calculatePath(
        glm::vec2 start, glm::vec2 end, glm::vec2 size, int maxiters);

    const TerrainTile getTileAtPosition(glm::vec2);

    /**
     * Move a node from the open to the closed list
     *
     * Obviously we remove it from the open list
     */
    void moveToClosedList(PathNode* n);

    /**
     * Check if node is not in an obstacle
     */
    bool isWalkable(const PathNode& n, glm::vec2 size) const;

    /**
     * Check if node is in the closed list
     */
    bool isInClosedList(const PathNode& n) const;

    /**
     * Check if the node is in the open list
     *
     * If yes, return a pointer to it
     * If not, return nullptr
     */
    PathNode* findInOpenList(const PathNode& n);

    /**
     * Generate a list of neighbors for a given node
     *
     * We generate exactly 8 neigbors, in all directions, like this:
     *
     *   N | N | N     (X is the given node, all others are the neighbors)
     *  ---+---+---
     *   N | X | N
     *  ---+---+---
     *   N | N | N
     *
     *  We might generate less neighbors if, for example, you are in the border of
     *  the map.
     *
     */
    std::vector<std::unique_ptr<Pathfinder::PathNode>> generateNeighbors(const PathNode& n);

    std::vector<glm::vec2> getCoordsInsideObject(glm::vec2 pos, glm::vec2 size) const;
};

};  // namespace familyline::logic
