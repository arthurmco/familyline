#pragma once

/**
 * Base player class, from whom all players will inherit from
 *
 * (C) 2020 Arthur Mendes
 */

#include <common/logic/game_object.hpp>
#include <common/logic/icamera.hpp>
#include <common/logic/object_manager.hpp>
#include <common/logic/player_actions.hpp>
#include <common/logic/terrain.hpp>
#include <functional>
#include <map>
#include <queue>
#include <stack>
#include <string_view>
#include <vector>

/// TODO: make the player manager handle the player actions, not the
/// input manager

namespace familyline::logic
{
class PlayerManager;

struct GameContext {
    ObjectManager* om      = nullptr;
    int tick               = 0;
    double elapsed_seconds = 0;
};

/// TODO: add player input actions for pushing and popping entity
/// action state, and to add and remove an action to said state

enum EntityActionMethod { Click, Activate };

/**
 * The entity action handler
 *
 * Returns a bool if the action was run, false if it was not
 *
 * Passes the originating game object, and a bool to signify if the
 * action is being activated or deactivated. If it is a click action
 * this value is always true
 */
using EntityActionHandler = std::function<bool(GameObject&, bool)>;

/**
 * An entity action is anything that a game object makes the player
 * able to do
 *
 * They usually appear in the action bar (the one with a lot of
 * buttons with icons).
 *
 * They can have a name, a description, an icon, a key combination
 * (as a string, that will be parsed) and a handler
 *
 * They also have the execution method: click and activate (the action
 * can be set on/off, like a switch). They also can be enabled or not.
 *
 * When the user strikes the key combination, or clicks on the icon,
 * the handler will run
 */
struct EntityAction {
    std::string name, description, iconpath, keycombo;

    EntityActionMethod eact_method;
    bool enabled;

    EntityActionHandler handler;
};

/**
 * The player class
 *
 * Handles the player actions
 *
 * To implement a player type, you might want to inherit this, but the only
 * methods you will need to implement are the input generation methods
 */
class Player
{
    friend class PlayerManager;

private:
    std::string name_;
    int code_;
    PlayerManager& pm_;

    /**
     * The build queue
     *
     * Holds the buildings that the player told the game to build
     * Units that are trained are not handled by this.
     *
     * TODO: check if is possible to treat building creation as a special
     * type of action
     *
     * The strings it holds are the type names of the buildings
     * the user wants to build
     */
    std::queue<std::string> build_queue_;

    /**
     * The entity action states
     *
     * They are individual for each player
     *
     * It is a stack of hashmaps. The stack is for easing pushing/popping
     * states, and the hashmap is to map grid position to actions
     *
     * The actions are drawn in the interface a grid, so this is good for knowing the
     * position
     */
    std::stack<std::map<int, EntityAction>> entity_actions_;

    std::optional<std::string> nextBuilding_;

protected:
    const Terrain& terr_;

    void pushAction(PlayerInputType type);
    void pushAction(PlayerInputType type, uint64_t tick);

    std::optional<ICamera*> camera_;
    std::vector<std::weak_ptr<GameObject>> selected_;

    /**
     * Get the current tick, as accounted by the player manager
     *
     * Since the player manager `run()` and the input generation function
     * `generateInput()` might not run intertwined (like, for example,
     * instead of 1 `run()` and 2 `generateInput()` for one tick, we will
     * have 20 `generateInput()`s first and then 10 `run()`s), if you want
     * to keep synchronized with the player manager, you will have to use
     * this function to get its stored tick count
     */
    size_t getTick();

    /**
     * Check if the logic and input functions are running in synchrony, one after
     * another.
     *
     * This is a good way to check if the value output by the getTick function is
     * trustworthy
     */
    bool isTickValid();

public:
    Player(PlayerManager& pm, const Terrain& terr, const char* name, int code)
        : pm_(pm), terr_(terr), name_(name), code_(code)
    {
    }

    unsigned int getCode() { return (unsigned)code_; }

    std::optional<std::string> getNextBuilding() const { return nextBuilding_; }
    void popNextBuilding() { nextBuilding_ = std::optional<std::string>(); }
    void pushNextBuilding(std::string b) { nextBuilding_ = std::optional<std::string>(b); }

    /**
     * Push a selection to the player selected items list
     *
     * The object_id is just for error reporting, you pass the object ID
     * of the game object you are passing
     */
    void pushToSelection(unsigned object_id, std::weak_ptr<GameObject>);

    void popFromSelection(unsigned object_id);

    void clearSelection() { selected_.clear(); }

    const std::vector<std::weak_ptr<GameObject>>& getSelections() const { return selected_; }

    /**
     * Generate the input actions.
     *
     * They must be pushed to the input manager
     */
    virtual void generateInput() = 0;

    /**
     * Does this player requested game exit?
     */
    virtual bool exitRequested() { return false; }

    /**
     * Process the player input actions
     *
     * They might do things like moving a unit, or attacking someone,
     * or running some action
     *
     * They will receive data from the input manager.
     */
    void processActions();

    std::string_view getName() { return this->name_; }

    std::optional<ICamera*> getCamera() const { return this->camera_; }

    virtual ~Player() {}
};

}  // namespace familyline::logic
