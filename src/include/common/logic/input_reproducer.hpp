#pragma once

/**
 * Reproduces a previously recorded input
 *
 * (C) 2020 Arthur Mendes
 */

#include <input_serialize_generated.h>

#include <array>
#include <common/logger.hpp>
#include <common/logic/player_actions.hpp>
#include <common/logic/player_session.hpp>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>
#include "input_generated.h"

namespace familyline::logic
{
struct InputInfo {
    std::string name;
    std::string color;
    uint64_t id;

    std::vector<uint64_t> allies;
    std::vector<uint64_t> enemies;
};

class ReplayPlayer;
class ObjectFactory;

class InputReproducer
{
public:
    InputReproducer(std::string_view file)
        : file_(file), f_(nullptr), pinfo_({}), actioncount_(0), currentaction_(0)
    {
    }

    /**
     * Open and verify the file
     *
     * It will return true on success, and false on error
     */
    bool open();

    /**
     * Verify the object checksums, compare them with the ones in the
     * file
     */
    bool verifyObjectChecksums(ObjectFactory* const of);

    /**
     * Create a player session with players that will
     * reproduce what happened in the file
     *
     * The diplomacy will be kept neutral between everyone, but the
     * correct thing to do is to save it into the file
     */
    PlayerSession createPlayerSession(Terrain& terrain);

    InputReproducer(InputReproducer&)       = delete;
    InputReproducer(const InputReproducer&) = delete;

    std::string getTerrainFile() { return "terrain_test.flte"; }

    /**
     * Get the next action from the file
     *
     * If no more actions exist, returns an empty optional
     */
    std::optional<PlayerInputAction> getNextAction();

    uint64_t getCurrentActionIndex() const { return currentaction_; }

    void reset()
    {
        if (f_) {
            fseek(f_, off_actionlist_, SEEK_SET);
            currentaction_ = 0;
        }
    }

    bool isReproductionEnded() const;

    /**
     * Dispatch events to the players, from nextTick_ to nextTick_+nextTicks
     */
    void dispatchEvents(unsigned nextTicks);

    ~InputReproducer();

private:
    std::string file_;
    FILE* f_;

    std::vector<InputInfo> pinfo_;

    using checksum_raw_t =
        std::vector<std::tuple<std::string /* type */, std::array<uint8_t, 256> /* checksum */>>;

    /// Checksum information, in a somewhat raw form
    checksum_raw_t pchecksum_;

    std::tuple<std::vector<InputInfo>, decltype(pchecksum_)> readPlayerInfo(FILE* file);

    off_t off_actionlist_ = 0;
    long long int actioncount_, currentaction_;

    void onActionEnd(ReplayPlayer* p);

    std::map<int, std::function<void(PlayerInputAction)>> action_callbacks_;
    std::map<int, bool> player_ended_;

    int nextTick_ = 0;
    std::optional<PlayerInputAction> last_action_;

    void dispatchAction(const PlayerInputAction& action);
};

/**
 * Parse a serialized input message and return its data, as a PlayerInputType
 *
 * Useful for both the input reproducer and the network packet parsing code.
 *
 * fnTypeCode is a function that returns an InputType_... code based on the action info
 * fnTypeData is a function that returns the data of said message in the inputtype.
 *
 * All of this to avoid duplicating code... But it will be worth it!
 */
template <typename SerialPtr, typename FnGetTypeCode, typename FnGetTypeData>
PlayerInputType deserializeInputAction(
    const SerialPtr* actioninfo, FnGetTypeCode&& fnTypeCode, FnGetTypeData&& fnTypeData)
{
    PlayerInputType type;
    auto& log = LoggerService::getLogger();

    auto atype = fnTypeCode(actioninfo);
    switch (atype) {
        case familyline::InputType_cmd: {
            auto cmd = (familyline::CommandInput*) fnTypeData(actioninfo, atype);
            auto val = std::monostate{};

            switch (cmd->args()->args()->size()) {
                case 0: type = CommandInput{cmd->command()->str(), std::monostate{}}; break;

                case 1:
                    type = CommandInput{cmd->command()->str(), cmd->args()->args()->Get(0)};
                    break;
                case 2:
                    type = CommandInput{
                        cmd->command()->str(),
                        std::array<int, 2>{
                            (int)cmd->args()->args()->Get(0), (int)cmd->args()->args()->Get(1)}};
                    break;
                default:
                    log->write(
                        "input-reproducer", familyline::LogType::Error,
                        "invalid parameter count for command (%zu)", cmd->args()->args()->size());
                    type = CommandInput{cmd->command()->str(), std::monostate{}};
                    break;
            }

        } break;
        case familyline::InputType_sel: {
            auto sel = (familyline::SelectAction*) fnTypeData(actioninfo, atype);
            std::vector<long unsigned int> objects;

            std::copy(
                sel->objects()->values()->cbegin(), sel->objects()->values()->cend(),
                std::back_inserter(objects));
            type = SelectAction{objects};
        } break;
        case familyline::InputType_obj_move: {
            auto omove = (familyline::ObjectMove*) fnTypeData(actioninfo, atype);
            int xPos = (int)omove->x_pos(), yPos = (int)omove->y_pos();

            type = ObjectMove{xPos, yPos};
        } break;
        case familyline::InputType_cam_move: {
            auto cmove = (familyline::CameraMove*) fnTypeData(actioninfo, atype);
            double dX = cmove->x_delta(), dY = cmove->y_delta(), dZoom = cmove->zoom_delta();

            type = CameraMove{dX, dY, dZoom};
        } break;
        case familyline::InputType_cam_rotate: {
            auto crot      = (familyline::CameraRotate*) fnTypeData(actioninfo, atype);
            double radians = crot->radians();

            type = CameraRotate{radians};
        } break;
        case familyline::InputType_create: {
            auto centity      = (familyline::CreateEntity*) fnTypeData(actioninfo, atype);
            std::string etype = centity->type()->str();
            int xPos = centity->x_pos(), yPos = centity->y_pos();

            type = CreateEntity{etype, xPos, yPos};
        } break;

        default: type = CommandInput{"null", 0ul}; break;
    }

    return type;
}

}  // namespace familyline::logic
