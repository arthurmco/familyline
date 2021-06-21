#include <input_serialize_generated.h>
#include <zlib.h>  // for the CRC32 calculation

#include <algorithm>
#include <array>
#include <cerrno>
#include <cinttypes>
#include <common/logger.hpp>
#include <common/logic/input_file.hpp>
#include <common/logic/input_reproducer.hpp>
#include <common/logic/object_factory.hpp>
#include <common/logic/player_actions.hpp>
#include <common/logic/player_manager.hpp>
#include <common/logic/replay_player.hpp>
#include <cstring>
#include <functional>
#include <optional>

#include "input_generated.h"

using namespace familyline::logic;

using checksum_raw_t =
    std::vector<std::tuple<std::string /* type */, std::array<uint8_t, 256> /* checksum */>>;

/**
 * Read player info at the file's current position
 */
std::tuple<std::vector<InputInfo>, checksum_raw_t> InputReproducer::readPlayerInfo(FILE* file)
{
    auto& log = LoggerService::getLogger();

    uint32_t size = 0;
    fread((void*)&size, 1, sizeof(size), file);

    char* data = new char[size + 1];
    auto rsize = fread((void*)data, 1, size, file);

    if (size != rsize) {
        log->write(
            "input-reproducer", LogType::Error, "Could not read the player info section (%d of %d)",
            rsize, size);

        delete[] data;
        return {};
    }

    flatbuffers::FlatBufferBuilder builder;

    auto playerinfo = flatbuffers::GetRoot<RecordHeader>((uint8_t*)data);

    std::vector<logic::InputInfo> players;
    for (auto vit = playerinfo->players()->cbegin(); vit != playerinfo->players()->cend(); ++vit) {
        log->write(
            "input-reproducer", LogType::Info, "found player (name %s, id %" PRIx64 ", color %s)",
            vit->name()->c_str(), vit->id(), vit->color()->c_str());

        players.push_back(
            logic::InputInfo{vit->name()->str(), vit->color()->str(), vit->id(), {}, {}});
    }

    checksum_raw_t checksuminfo;
    auto serchecksums = playerinfo->checksums();

    std::vector<std::string> typenames;
    std::vector<std::array<uint8_t, 256>> typechecksums;

    if (!serchecksums) {
        log->write(
            "input-reproducer", LogType::Warning, "no checksum information found on this file");
        return std::tie(players, checksuminfo);
    }

    for (auto vit = serchecksums->typenames()->cbegin(); vit != serchecksums->typenames()->cend();
         ++vit) {
        puts(vit->c_str());
        typenames.push_back(vit->str());
    }

    for (auto vit = serchecksums->checksums()->cbegin(); vit != serchecksums->checksums()->cend();
         ++vit) {
        std::array<uint8_t, 256> r;
        std::copy(vit->value()->cbegin(), vit->value()->cend(), r.begin());

        typechecksums.push_back(r);
    }

    int i = 0;
    for (auto name : typenames) {
        checksuminfo.push_back(std::make_tuple(name, typechecksums[i]));
        i++;
    }

    delete[] data;
    return std::tie(players, checksuminfo);
}

long long int readInputCount(FILE* file)
{
    using namespace familyline;

    auto& log = LoggerService::getLogger();

    auto loc = ftell(file);

    uint32_t icount      = 0;
    unsigned footerstart = 12;  // MAGIC + inputcount + checksum

    fseek(file, 0, SEEK_END);
    auto end = ftell(file);

    fseek(file, end - footerstart, SEEK_SET);
    char magic[5] = {};

    fread((void*)magic, 4, 1, file);
    if (strncmp(magic, R_FOOTER_MAGIC, 4)) {
        log->write(
            "input-reproducer", LogType::Error, "Wrong footer section magic (%s != %s)", magic,
            R_FOOTER_MAGIC);
        fseek(file, loc, SEEK_SET);
        return -1;
    }

    fread((void*)&icount, 1, sizeof(icount), file);

    fseek(file, loc, SEEK_SET);
    return (long long int)(icount & 0xffffffff);
}

/**
 * Verify if the file checksum is OK.
 *
 * Returns true if the checksum matches, false if it does not
 *
 * There will be files that will not have the checksum field, but a tool
 * will be built to repair them
 */
bool verifyChecksum(FILE* f)
{
    auto pos = ftell(f);
    fseek(f, 0L, SEEK_END);

    auto filesize = ftell(f);

    rewind(f);
    auto checksumpos = filesize - 4;
    char* filedata   = new char[filesize];

    auto reallen              = fread(filedata, 1, filesize, f);
    filedata[checksumpos]     = 0;
    filedata[checksumpos + 1] = 0;
    filedata[checksumpos + 2] = 0;
    filedata[checksumpos + 3] = 0;

    unsigned long crc = crc32(0L, Z_NULL, 0);
    crc               = crc32(crc, (const unsigned char*)filedata, reallen);

    delete[] filedata;

    uint32_t file_crc = 0;

    fseek(f, checksumpos, SEEK_SET);
    fread(&file_crc, 1, sizeof(file_crc), f);

    bool ret = file_crc == crc;

    fseek(f, pos, SEEK_SET);

    return ret;
}

/**
 * Open and verify the file
 *
 * It will return true on success, and false on error
 */
bool InputReproducer::open()
{
    auto& log = LoggerService::getLogger();

    f_ = fopen(file_.data(), "rb");
    if (!f_) {
        f_ = nullptr;
        log->write(
            "input-reproducer", LogType::Error, "Could not open the input file %s (error %d: %s)",
            file_.data(), errno, strerror(errno));

        return false;
    }

    char magic[5]    = {};
    uint32_t version = 0;
    fread((void*)magic, 1, 4, f_);
    fread((void*)&version, 1, 4, f_);

    if (strncmp(magic, R_MAGIC, 4)) {
        log->write(
            "input-reproducer", LogType::Error, "Invalid magic value for file %s (FREC != %s)",
            file_.data(), magic);

        return false;
    }

    if (version != R_VERSION) {
        log->write(
            "input-reproducer", LogType::Error, "Incompatible file version file %s (%x != %x)",
            file_.data(), R_VERSION, version);

        return false;
    }

    if (!verifyChecksum(f_)) {
        log->write("input-reproducer", LogType::Error, "Invalid checksum of file %s", file_.data());

        return false;
    }

    std::tie(pinfo_, pchecksum_) = readPlayerInfo(f_);
    if (pinfo_.size() == 0) {
        return false;
    }

    actioncount_ = readInputCount(f_);
    if (actioncount_ < 0) {
        return false;
    }

    log->write(
        "input-reproducer", LogType::Info, "'%s': %lld input actions detected", file_.data(),
        actioncount_);

    off_actionlist_ = ftell(f_);
    currentaction_  = 0;

    return true;
}

void InputReproducer::onActionEnd(ReplayPlayer* p)
{
    if (currentaction_ == actioncount_) {
        player_ended_[p->getCode()] = true;
    }
}

bool InputReproducer::isReproductionEnded() const
{
    return std::all_of(player_ended_.begin(), player_ended_.end(), [](auto p) { return p.second; });
}

/**
 * Verify the object checksums, compare them with the ones in the
 * file
 *
 */
bool InputReproducer::verifyObjectChecksums(ObjectFactory* const of)
{
    auto& log = LoggerService::getLogger();

    auto checksumlist = of->getObjectChecksums();
    if (pchecksum_.size() == 0) {
        log->write(
            "input-reproducer", LogType::Warning,
            "input file %s has no object checksum field. We cannot determine what the game will "
            "use",
            file_.data());
        log->write(
            "input-reproducer", LogType::Warning, "It is better not to continue read the file");
        return false;
    }

    if (checksumlist.size() != pchecksum_.size()) {
        log->write(
            "input-reproducer", LogType::Warning,
            "input file %s and this game has different count of objects (%zu here, %zu in the "
            "file)",
            file_.data(), checksumlist.size(), pchecksum_.size());
        log->write(
            "input-reproducer", LogType::Warning,
            "The simulation will work, do not worry, but be aware");
    }

    std::vector<bool> foundChecksums(pchecksum_.size(), false);

    int i = 0;
    for (auto& craw : pchecksum_) {
        auto stype    = std::get<0>(craw);
        auto checksum = std::get<1>(craw);

        if (checksumlist[stype] != checksum) {
            log->write(
                "input-reproducer", LogType::Error,
                "input file %s has a different checksum of building type '%s'", file_.data(),
                stype.c_str());
            return false;
        }

        foundChecksums[i] = true;
        i++;
    }

    if (std::find(foundChecksums.begin(), foundChecksums.end(), false) != foundChecksums.end()) {
        log->write(
            "input-reproducer", LogType::Error,
            "we have some objects in the file '%s' that were not in this game", file_.data());
        return false;
    }

    return true;
}

/**
 * Create a player session with players that will
 * reproduce what happened in the file
 *
 * The diplomacy will be kept neutral between everyone, but the
 * correct thing to do is to save it into the file
 */
PlayerSession InputReproducer::createPlayerSession(Terrain& terrain)
{
    std::map<uint64_t /*player_id*/, std::reference_wrapper<logic::Colony>> player_colony;
    auto pm = std::make_unique<PlayerManager>();
    auto cm = std::make_unique<ColonyManager>();

    for (auto& p : pinfo_) {
        auto code = p.id;

        {            
            ReplayPlayer* rp = new ReplayPlayer{
                *pm.get(),
                terrain,
                p.name.c_str(),
                code,
                *this,
                std::bind(&InputReproducer::onActionEnd, this, std::placeholders::_1)};
            action_callbacks_.insert(
                {code, std::bind(&ReplayPlayer::enqueueAction, rp, std::placeholders::_1)});
            player_ended_.insert({code, false});

            pm->add(std::unique_ptr<Player>(rp), false);
        }

        int r = 0, g = 0, b = 0;
        sscanf(p.color.c_str(), "%02x%02x%02x", &r, &g, &b);
        int colorval = (b & 0xff) | (g << 8) | (b << 16);

        auto* player   = *(pm->get(code));
        auto& alliance = cm->createAlliance(p.name);
        auto& colony   = cm->createColony(
            *player, colorval, std::optional<std::reference_wrapper<Alliance>>{alliance});

        player_colony.emplace(p.id, std::reference_wrapper(colony));
    }

    return PlayerSession{std::move(pm), std::move(cm), player_colony};
}

/**
 * Get the next action from the file
 *
 * If no more actions exist, returns an empty optional
 */
std::optional<PlayerInputAction> InputReproducer::getNextAction()
{
    if (currentaction_ == actioncount_) return std::nullopt;

    // action structure: string "FINP" + int containg the size + flatbuffer data
    // of the input action.
    auto& log = LoggerService::getLogger();

    auto apos = ftell(f_);

    char magic[5]       = {};
    uint32_t actionsize = 0;

    fread((void*)magic, 4, 1, f_);
    if (strncmp(magic, R_ACTION_MAGIC, 4)) {
        log->write(
            "input-reproducer", LogType::Fatal,
            "'%s': Action number %d (at offset %08x) is invalid", file_.data(), currentaction_,
            apos);

        return std::nullopt;
    }

    fread((void*)&actionsize, sizeof(actionsize), 1, f_);
    if (actionsize == 0) {
        log->write(
            "input-reproducer", LogType::Fatal,
            "'%s': Action number %d (at offset %08x) has no size. Stopping because this is very "
            "weird",
            file_.data(), currentaction_, apos);

        return std::nullopt;
    }

    char* data = new char[actionsize + 1];
    auto rsize = fread((void*)data, 1, actionsize, f_);

    flatbuffers::FlatBufferBuilder builder;
    auto actioninfo = flatbuffers::GetRoot<InputElement>((uint8_t*)data);

    PlayerInputType type = deserializeInputAction(
        actioninfo, [](const InputElement* a) { return a->type_type(); },
        [&](const InputElement* a, familyline::InputType type) {
            switch (type) {
            case familyline::InputType_cmd: return (void*)a->type_as_cmd();
            case familyline::InputType_sel: return (void*)a->type_as_sel();
            case familyline::InputType_obj_move: return (void*)a->type_as_obj_move();
            case familyline::InputType_cam_move: return (void*)a->type_as_cam_move();
            case familyline::InputType_cam_rotate: return (void*)a->type_as_cam_rotate();
            case familyline::InputType_create: return (void*)a->type_as_create();
            default:
                log->write("input-reproducer", LogType::Fatal, "Invalid input message type (%02x)", type);
                return (void*)nullptr;
            }
        });

    auto val = std::optional<PlayerInputAction>{PlayerInputAction{
        actioninfo->timestamp(), actioninfo->playercode(), (uint32_t)actioninfo->tick(), type}};

    currentaction_++;

    delete[] data;
    return val;
}

void InputReproducer::dispatchAction(const PlayerInputAction& action)
{
    if (auto it = action_callbacks_.find(action.playercode); it != action_callbacks_.end()) {
        it->second(action);
    }
}

/**
 * Dispatch events to the players, from nextTick_ to nextTick_+nextTicks
 */
void InputReproducer::dispatchEvents(unsigned nextTicks)
{
    auto endTick = nextTick_ + nextTicks;
    do {
        if (last_action_) {
            this->dispatchAction(*last_action_);
        }
        last_action_ = this->getNextAction();
    } while (last_action_ && last_action_->tick <= endTick);

    nextTick_ = endTick;
}

InputReproducer::~InputReproducer()
{
    if (f_) {
        fclose(f_);
    }
}
