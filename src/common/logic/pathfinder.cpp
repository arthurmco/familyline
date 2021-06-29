#include <common/logger.hpp>
#include <common/logic/pathfinder.hpp>
#include <glm/geometric.hpp>
#include <thread>

using namespace familyline::logic;

#include <algorithm>
#include <iterator>

const Pathfinder::TerrainTile Pathfinder::getTileAtPosition(glm::vec2 p)
{
    auto [width, height] = t_.getSize();
    auto idx             = int(p.y) * width + int(p.x);

    return TerrainTile{t_.getHeightFromCoords(p), (TerrainType&)t_.getTypeData()[idx]};
}

/**
 * Sort the list so that the best tiles stay in the top
 */
Pathfinder::PathNode* sortBestTile(std::list<std::unique_ptr<Pathfinder::PathNode>>& open_list)
{
    open_list.sort([](const std::unique_ptr<Pathfinder::PathNode>& a,
                      const std::unique_ptr<Pathfinder::PathNode>& b) { return a->f() < b->f(); });
    return open_list.front().get();
}

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
 *  the map, and the neighbor would be out of the map (less than 0, or more than
 *  the width/height)
 *
 */
std::vector<std::unique_ptr<Pathfinder::PathNode>> Pathfinder::generateNeighbors(const PathNode& n)
{
    auto [width, height] = t_.getSize();

    std::vector<glm::vec2> directions = {{-ratio_, -ratio_}, {0, -ratio_},    {ratio_, -ratio_},
                                         {-ratio_, 0},       {ratio_, 0},     {-ratio_, ratio_},
                                         {0, ratio_},        {ratio_, ratio_}};

    std::vector<std::unique_ptr<Pathfinder::PathNode>> ret;
    for (auto& d : directions) {
        auto newpos = d + n.position;
        assert(int(newpos.x) % ratio_ == 0);
        assert(int(newpos.y) % ratio_ == 0);

        if (newpos.x < 0 || newpos.y < 0) continue;

        if (newpos.x >= width || newpos.y >= height) continue;

        ret.push_back(std::make_unique<PathNode>(newpos));
    }
    return ret;
}

/**
 * Check if the node is in the open list
 *
 * If yes, return a pointer to it
 * If not, return nullptr
 */
Pathfinder::PathNode* Pathfinder::findInOpenList(const PathNode& n)
{
    auto it = std::find_if(
        open_list_.begin(), open_list_.end(),
        [&](const std::unique_ptr<Pathfinder::PathNode>& v) { return v->position == n.position; });

    if (it == open_list_.end()) return nullptr;

    return it->get();
}

/**
 * Find all the coordinates that are considered inside the object, respecting the
 * grid
 */
std::vector<glm::vec2> Pathfinder::getCoordsInsideObject(glm::vec2 pos, glm::vec2 size) const
{
    std::vector<glm::vec2> coords;
    assert(size.x > 0);
    assert(size.y > 0);
    auto [width, height] = t_.getSize();

    /// The object position is at the center
    auto minx = glm::round(pos.x - (size.x / 2.0));
    auto maxx = glm::round(pos.x + (size.x / 2.0));
    auto miny = glm::round(pos.y - (size.y / 2.0));
    auto maxy = glm::round(pos.y + (size.y / 2.0));
    std::vector<int> indices;
    for (auto y = int(miny); y <= int(maxy); y++) {
        for (auto x = int(minx); x <= int(maxx); x++) {
            if (x >= 0 && y >= 0 && x < width && y < height) coords.push_back(glm::vec2(x, y));
        }
    }

    return coords;
}

/**
 * Check if node is not in an obstacle
 *
 * Consider the object size.
 * Imagine that the object position is at the center, and the size
 * delimits the width and height.
 *
 * Like this:
 *
 *   size.x
 *  |-----|_
 *   ooooo |
 *   ooPoo | size.y
 *   ooooo |
 *         -
 */
bool Pathfinder::isWalkable(const PathNode& n, glm::vec2 size) const
{
    auto width  = std::get<0>(t_.getSize());
    auto coords = getCoordsInsideObject(n.position, size);
    std::vector<unsigned int> indices;
    auto ratio = ratio_;
    std::transform(
        coords.begin(), coords.end(), std::back_inserter(indices),
        [width, ratio](auto& v) { return int(v.y / ratio) * (width / ratio) + int(v.x / ratio); });

    if (indices.size() == 0) return false;

    return std::all_of(indices.begin(), indices.end(), [&](auto index) {
        return (index >= obstacle_bitmap_.size()) ? false : !obstacle_bitmap_[index];
    });
}

/**
 * Check if node is in the closed list
 */
bool Pathfinder::isInClosedList(const PathNode& n) const
{
    auto it = std::find_if(
        closed_list_.rbegin(), closed_list_.rend(),
        [&](const std::unique_ptr<Pathfinder::PathNode>& v) { return v->position == n.position; });
    return (it != closed_list_.rend());
}

void Pathfinder::moveToClosedList(Pathfinder::PathNode* n)
{
    auto ptr = std::find_if(
        open_list_.begin(), open_list_.end(),
        [&](std::unique_ptr<PathNode>& node) { return node.get() == n; });
    assert(ptr != open_list_.end());

    auto distance = std::distance(open_list_.begin(), ptr);
    std::unique_ptr<PathNode> nnode;
    nnode = std::move(*ptr);

    open_list_.erase(std::next(open_list_.begin(), distance));

    closed_list_.push_back(std::move(nnode));
}

/// Purge the list of non-existing pointers or pointers that point to nothing
void purgeList(std::list<std::unique_ptr<Pathfinder::PathNode>>& list)
{
    list.erase(
        std::remove_if(
            list.begin(), list.end(),
            [](std::unique_ptr<Pathfinder::PathNode>& v) { return v ? false : true; }),
        list.end());
}

Pathfinder::PathNode* Pathfinder::traversePath(
    glm::vec2 start, glm::vec2 end, glm::vec2 size, int maxiters)
{
    auto nstart = std::make_unique<PathNode>(start);
    nstart->calculateValues(start, end, getTileAtPosition(nstart->position));
    open_list_.push_back(std::move(nstart));
    has_max_iter_reached_ = false;

    /// If the start does not align to the "grid", we make it align
    if (int(start.x) % ratio_ != 0 || int(start.y) % ratio_ != 0) {
        auto direction = end - start;
        auto vec       = glm::vec2(
            direction.x > 0 ? 1 / float(ratio_) : (direction.x < 0 ? -1 / float(ratio_) : 0),
            direction.y > 0 ? 1 / float(ratio_) : (direction.y < 0 ? -1 / float(ratio_) : 0));

        if (int(start.x) % ratio_ == 0) vec.x = 0;
        if (int(start.y) % ratio_ == 0) vec.y = 0;

        bool walkable                 = false;
        int idx                       = 0;
        std::array<glm::vec2, 8> vals = {glm::vec2(0, 0),  glm::vec2(0, 1),  glm::vec2(0, -1),
                                         glm::vec2(1, 0),  glm::vec2(-1, 0), glm::vec2(1, 1),
                                         glm::vec2(-1, -1)};

        moveToClosedList(open_list_.back().get());
        
        do {
            auto pos = (start / float(ratio_) + (vec + vals[idx])) * float(ratio_);

            auto alignstart = std::make_unique<PathNode>(pos);
            walkable        = isWalkable(*alignstart.get(), size);

            alignstart->calculateValues(start, end, getTileAtPosition(alignstart->position));
            alignstart->parent = closed_list_.back().get();

            if (walkable) {
                open_list_.push_back(std::move(alignstart));
            } else {
                idx++;
                if (idx > vals.size()) {
                    LoggerService::getLogger()->write(
                        "pathfinder", LogType::Warning,
                        "Fractional path is completely blocked! Cannot pass through! Returning the "
                        "best value");
                    return closed_list_.back().get();
                }
            }
        } while (!walkable);
    }

    int itercount = 0;
    auto endtiles = getCoordsInsideObject(end, size);
    if (endtiles.size() == 0) {
        LoggerService::getLogger()->write(
            "pathfinder", LogType::Warning,
            "cannot go there (%.2f, %.2f)! the pathfinder will try to go to the nearest place",
            end.x, end.y);
    }

    while (!open_list_.empty()) {
        PathNode* best = sortBestTile(open_list_);
        moveToClosedList(best);

        // we are in the final position
        if (glm::round(best->position) == glm::round(end)) {
            break;
        }

        // we are not in the final position, but sufficiently close to
        if (auto delta = glm::abs(end - best->position);
            delta.x < double(ratio_) && delta.y < double(ratio_)) {
            auto nend    = std::make_unique<PathNode>(end);
            nend->parent = closed_list_.back().get();
            closed_list_.push_back(std::move(nend));
            break;
        }

        // we cannot go to the final position, but we are sufficiently close
        if (PathNode tend{end}; !isWalkable(tend, size) &&
                                std::any_of(endtiles.begin(), endtiles.end(), [&](auto& endpos) {
                                    return endpos == best->position;
                                })) {
            LoggerService::getLogger()->write(
                "pathfinder", LogType::Warning,
                "requested end point (%.2f, %.2f) not equal to found end point (%.2f, %.2f), but "
                "close enough",
                end.x, end.y, best->position.x, best->position.y);
            break;
        }

        // we exceeded the iteration count, maybe retry again next tick
        if (itercount == maxiters) {
            LoggerService::getLogger()->write(
                "pathfinder", LogType::Info, "tick count exceeded! Repathing on next call");
            has_max_iter_reached_ = true;
            break;
        }

        // we are not in the final position nor exceeding iter count
        auto neighbors = generateNeighbors(*best);
        for (auto& n : neighbors) {
            n->parent = best;
            n->calculateValues(start, end, getTileAtPosition(n->position));
            if (!isWalkable(*n.get(), size) || isInClosedList(*n.get())) continue;

            auto existing = findInOpenList(*n.get());
            if (existing) {
                if (existing->g < n->g) {
                    existing->parent = best;
                    existing->calculateG(start, end, getTileAtPosition(existing->position));
                }

                continue;
            }

            std::unique_ptr<PathNode> npointer;
            npointer.swap(n);
            open_list_.push_back(std::move(npointer));
        }

        purgeList(open_list_);
        purgeList(closed_list_);

        LoggerService::getLogger()->write(
            "pathfinder", LogType::Debug,
            "(%03d) open list has %zu, closed list has %zu, best: (%.2f, %.2f)", itercount,
            open_list_.size(), closed_list_.size(), best->position.x, best->position.y);

        itercount++;
    }

    if (open_list_.empty()) {
        LoggerService::getLogger()->write(
            "pathfinder", LogType::Warning,
            "Path is completely blocked! Cannot pass through! Returning the best value");
    }

    return closed_list_.back().get();
}

void Pathfinder::PathNode::calculateG(glm::vec2 start, glm::vec2 end, const TerrainTile& current)
{
    if (parent) {
        auto heightcost = glm::abs(glm::distance(double(current.height), parent->height)) * 0.01;
        g = parent->g + glm::abs(glm::distance(position, parent->position)) + heightcost;
    } else {
        g = 0;
    }
}

void Pathfinder::PathNode::calculateValues(
    glm::vec2 start, glm::vec2 end, const TerrainTile& current)
{
    this->calculateG(start, end, current);
    h      = glm::abs(glm::distance(position, end));
    height = current.height;
}

std::vector<glm::vec2> Pathfinder::calculatePath(
    glm::vec2 start, glm::vec2 end, glm::vec2 size, int maxiters)
{
    PathNode* node = traversePath(start, end, size, maxiters);
    assert(node);

    std::vector<glm::vec2> positions;
    positions.reserve(int(glm::abs(glm::distance(start, end))));
    for (PathNode* current = node; current != nullptr; current = current->parent) {
        if (positions.size() > 0 &&
            glm::abs(glm::distance(current->position, positions.back())) >= ratio_) {
            auto begin = positions.back();
            for (auto i = 1; i <= ratio_; i++) {
                positions.push_back(glm::mix(begin, current->position, i / double(ratio_)));
            }
        } else {
            positions.push_back(current->position);
        }
    }

    return positions;
}

std::vector<glm::vec2> Pathfinder::findPath(
    glm::vec2 start, glm::vec2 end, glm::vec2 size, int maxiters)
{
    LoggerService::getLogger()->write(
        "pathfinder", LogType::Debug,
        "trying to find a path between (%.2f, %.2f) and (%.2f, %.2f), with size (%.2f, %.2f)",
        start.x, start.y, end.x, end.y, size.x, size.y);

    assert(obstacle_bitmap_.size() > 1);

    auto positions = this->calculatePath(start, end, size, maxiters);
    std::vector<glm::vec2> ret;
    ret.reserve(positions.size());

    std::copy(positions.rbegin(), positions.rend(), std::back_inserter(ret));
    return ret;
}

void Pathfinder::update(std::vector<bool> bitmap, int ratio)
{
    auto [width, height] = t_.getSize();

    assert(bitmap.size() == (width / ratio) * (height / ratio));
    obstacle_bitmap_ = bitmap;

    // Invalidate both, because they are simply not valid now.
    //
    // We will need to calculate the whole path
    open_list_.clear();
    closed_list_.clear();
    ratio_ = ratio;
}
