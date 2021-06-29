#pragma once

/**
 * Control player input actions
 *
 * Since they depend on the player, not on the input, and the player info
 * lives on the logic module, they will go here, and not on the input module
 *
 * (C) 2020 Arthur M.
 */

#include <array>
#include <common/logic/types.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace familyline::logic
{
/**
 * Select one or more objects
 *
 * If two actions of this type come one after another, it will effectively
 * select all objects of the first, deselect and then select all objects
 * of the second.
 *
 * Therefore, if you need to deselect everyone, just send a select action
 * with no objects.
 */
struct SelectAction {
    std::vector<object_id_t> objects;
};

/**
 * Same as above, but just pushes the specified objects
 * to the current selection buffer
 */
struct AddSelectAction {
    std::vector<object_id_t> objects;
};

/**
 * Create a selection group with number `number`, with the current selected objects
 */
struct CreateSelectGroup {
    int number;
};

/**
 * Select all objects from the group `number`
 */
struct SelectGroup {
    int number;
};

/**
 * Controls a command
 *
 * An command is just a command that is created from a
 * building, or an unit
 * This command might cause an unit to set some state, or another
 * building to be created
 *
 * This command is run from all selected objects.
 * If you need to run a command in an object, you will need to select it first.
 *
 * The command can have an optional parameter, being an ID or a location.
 */
struct CommandInput {
    std::string commandName;
    std::variant<std::monostate, object_id_t, std::array<int, 2>> param;
};

/**
 * Creates an entity with type `type` in the specified coordinates
 *
 * Note that you should only send this command for initial entities
 * and entities that are created because of triggers, NEVER because
 * of an action
 */
struct CreateEntity {
    std::string type;
    int xPos, yPos;
};

/**
 * Makes one or more selected objects move to a specific location
 *
 * It only tells the final destination of the pathfinder, for example, not the
 * midpoints.
 * It was like it sent only the location of the "right-click"
 */
struct ObjectMove {
    int xPos;
    int yPos;
};

/**
 * Add the current selected objects to the group name `number`
 */
struct AddSelectGroup {
    int number;
};

/**
 * Removes the group `number`
 */
struct RemoveSelectGroup {
    int number;
};

/**
 * Controls camera positioning
 *
 * Might be useful on recorded games, to see and study where the player
 * focus on.
 */
struct CameraMove {
    double deltaX, deltaY;
    double deltaZoom;
};

/**
 * Controls camera rotation
 *
 * Also might be useful on recorded games.
 */
struct CameraRotate {
    double radians;
};

using PlayerInputType = std::variant<
    CommandInput, ObjectMove, SelectAction, AddSelectAction, CreateSelectGroup, CreateEntity,
    SelectGroup, AddSelectGroup, RemoveSelectGroup, CameraMove, CameraRotate>;

struct PlayerInputAction {
    uint64_t timestamp;
    uint64_t playercode;
    uint32_t tick;

    PlayerInputType type;
};

}  // namespace familyline::logic
