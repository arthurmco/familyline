#include <flatbuffers/flatbuffers.h>
#include <fmt/format.h>
#include <input_generated.h>
#include <input_serialize_generated.h>
#include <zlib.h>

#include <cerrno>
#include <cinttypes>
#include <common/logger.hpp>
#include <common/logic/input_file.hpp>
#include <common/logic/input_recorder.hpp>
#include <common/logic/object_factory.hpp>
#include <filesystem>

using namespace familyline::logic;

InputRecorder::InputRecorder(PlayerManager& pm) : pm_(pm)
{
    pm_handle_ = pm_.addListener(std::bind(&InputRecorder::addAction, this, std::placeholders::_1));

    auto pmap = pm_.getPlayerNames();
    for (auto [id, name] : pmap) {
        pinfo_.push_back(RecordPlayerInfo{(uint64_t)id, name});
    }
}

flatbuffers::Offset<familyline::ObjectChecksums> serializeChecksums(
    ObjectFactory* const of, flatbuffers::FlatBufferBuilder& builder)
{
    std::vector<flatbuffers::Offset<flatbuffers::String>> typenames;
    std::vector<flatbuffers::Offset<familyline::ObjectChecksum>> checksums;

    for (auto [type, checksum] : of->getObjectChecksums()) {
        auto sertype     = builder.CreateString(type);
        auto serchecksum = builder.CreateVector(checksum.data(), checksum.size());
        auto serval      = familyline::CreateObjectChecksum(builder, serchecksum);

        typenames.push_back(sertype);
        checksums.push_back(serval);
    }

    auto sertypenames = builder.CreateVector(typenames);
    auto serchecksums = builder.CreateVector(checksums);

    auto ret = familyline::CreateObjectChecksums(builder, sertypenames, serchecksums);
    return ret;
}

/**
 * Create the file.
 *
 * Return true if it could create, false if it could not
 */
bool InputRecorder::createFile(std::string_view path, ObjectFactory* const of)
{
    if (f_) fclose(f_);
    flatbuffers::FlatBufferBuilder builder;

    if (std::filesystem::is_regular_file(path)) {
        auto oldpath = path;
        path         = fmt::format("new-{}", path);
        LoggerService::getLogger()->write(
            "input-recorder", LogType::Error, "file '%s' already exists!, renaming to '%s'",
            oldpath.data(), path.data());
    }

    f_ = fopen(path.data(), "wb+");
    if (!f_) {
        LoggerService::getLogger()->write(
            "input-recorder", LogType::Error, "could not create input recorder file %s: %s (%d)",
            path.data(), strerror(errno), errno);
        path_ = "";
        return false;
    }

    // Add a header and a player list
    const char* magic = R_MAGIC;
    uint32_t version  = R_VERSION;

    fwrite((void*)magic, 1, 4, f_);
    fwrite((void*)&version, sizeof(version), 1, f_);

    std::vector<flatbuffers::Offset<familyline::PlayerInfo>> playerlist;

    for (auto& pi : pinfo_) {
        auto piName  = builder.CreateString(pi.name);
        auto piColor = builder.CreateString("#ff0000");
        auto piFlat  = CreatePlayerInfo(builder, pi.id, piName, piColor);

        playerlist.push_back(piFlat);
    }

    auto playervec   = builder.CreateVector(playerlist.data(), playerlist.size());
    auto checksums   = serializeChecksums(of, builder);
    auto playerchunk = CreateRecordHeader(builder, playervec, checksums);
    builder.Finish(playerchunk);
    uint32_t psize = builder.GetSize();
    fwrite(&psize, sizeof(psize), 1, f_);
    fwrite(builder.GetBufferPointer(), builder.GetSize(), 1, f_);

    if (!f_) return false;

    LoggerService::getLogger()->write(
        "input-recorder", LogType::Info, "creating input file %s", path.data());
    path_ = path;
    return true;
}

/// This will allow us to use std::visit with multiple variants at once, a thing
/// that should be part of C++20.
/// There are FOUR locations that I use this. I hope this gets suggested for C++23
/// or C++26
template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

/**
 * Convert a PlayerInputAction to its serialized representation as a flatbuffer
 *
 * This can be used in the input recorder as well as in the network input parser.
 */
void familyline::logic::serializeInputAction(
    const PlayerInputType& pit, familyline::InputType& type_val,
    flatbuffers::Offset<void>& type_data, flatbuffers::FlatBufferBuilder& builder)
{
    std::visit(
        overload{
            [&](const CommandInput& a) {
                auto cstr = builder.CreateString(a.commandName);

                std::vector<uint64_t> params;
                if (auto* objectID = std::get_if<object_id_t>(&a.param); objectID) {
                    params = {*objectID};
                } else if (auto* arr = std::get_if<std::array<int, 2>>(&a.param); arr) {
                    params = {(unsigned long)(*arr)[0], (unsigned long)(*arr)[1]};
                }

                auto pserialize = builder.CreateVector(params);
                auto cargs      = familyline::CreateCommandInputA(builder, pserialize);

                auto cval = CreateCommandInput(builder, cstr, cargs);

                type_val  = familyline::InputType_cmd;
                type_data = cval.Union();
            },
            [&](const SelectAction& a) {
                std::vector<uint64_t> cobjs;
                std::transform(
                    a.objects.begin(), a.objects.end(), std::back_inserter(cobjs),
                    [](auto v) { return v; });

                auto ovec  = builder.CreateVector(cobjs);
                auto oobjs = familyline::CreateSelectActionObjects(builder, ovec);
                auto cval  = CreateSelectAction(builder, oobjs);

                type_val  = familyline::InputType_sel;
                type_data = cval.Union();
            },
            [&](const ObjectMove& a) {
                auto cval = familyline::CreateObjectMove(builder, a.xPos, a.yPos);

                type_val  = familyline::InputType_obj_move;
                type_data = cval.Union();
            },
            [&](const CameraMove& a) {
                auto cval = familyline::CreateCameraMove(builder, a.deltaX, a.deltaY, a.deltaZoom);

                type_val  = familyline::InputType_cam_move;
                type_data = cval.Union();
            },
            [&](const CameraRotate& a) {
                auto cval = familyline::CreateCameraRotate(builder, a.radians);

                type_val  = familyline::InputType_cam_rotate;
                type_data = cval.Union();
            },
            [&](const CreateEntity& a) {
                auto strtype = builder.CreateString(a.type);
                auto cval    = familyline::CreateCreateEntity(builder, strtype, a.xPos, a.yPos);

                type_val  = familyline::InputType_create;
                type_data = cval.Union();
            },
            [&](const AddSelectAction& a) {

            },
            [&](const CreateSelectGroup& a) {}, [&](const SelectGroup& a) {},
            [&](const AddSelectGroup& a) {}, [&](const RemoveSelectGroup& a) {}},
        pit);
}

/**
 * You should pass this function as a callback to the player manager
 * `addListener` function
 */
bool InputRecorder::addAction(PlayerInputAction pia)
{
    if (f_ && path_ != "") {
        flatbuffers::FlatBufferBuilder builder;

        familyline::InputType type_val;
        flatbuffers::Offset<void> type_data;

        serializeInputAction(pia.type, type_val, type_data, builder);

        auto inputel = CreateInputElement(
            builder, pia.tick, pia.playercode, pia.timestamp, type_val, type_data);

        builder.Finish(inputel);

        uint32_t isize     = builder.GetSize();
        const char* imagic = R_ACTION_MAGIC;
        fwrite((void*)imagic, 1, 4, f_);
        fwrite(&isize, sizeof(isize), 1, f_);
        fwrite(builder.GetBufferPointer(), builder.GetSize(), 1, f_);

        inputcount_++;
    }

    return (f_ != nullptr);
}

uint32_t calculateChecksum(std::string path)
{
    FILE* firead = fopen(path.c_str(), "rb");
    fseek(firead, 0L, SEEK_END);
    auto filesize = ftell(firead);

    rewind(firead);
    char* filedata = new char[filesize];
    auto reallen   = fread(filedata, 1, filesize, firead);

    unsigned long crc = crc32(0L, Z_NULL, 0);
    crc               = crc32(crc, (const unsigned char*)filedata, reallen);

    delete[] filedata;
    fclose(firead);

    return crc;
}

void InputRecorder::commit()
{
    std::vector<flatbuffers::Offset<familyline::InputElement>> inputs;

    const char* endmagic = R_FOOTER_MAGIC;
    uint32_t inputcount  = inputcount_;
    uint32_t checksum    = 0;

    if (f_) {
        fwrite((void*)endmagic, 1, 4, f_);
        fwrite(&inputcount, sizeof(inputcount), 1, f_);

        auto checksumpos = ftell(f_);
        fwrite(&checksum, sizeof(checksum), 1, f_);
        fflush(f_);

        checksum = calculateChecksum(path_);
        LoggerService::getLogger()->write(
            "input-recorder", LogType::Info, "writing checksum %08x to the file %s", checksum,
            path_.c_str());

        fseek(f_, checksumpos, SEEK_SET);
        fwrite(&checksum, sizeof(checksum), 1, f_);
    }
}

InputRecorder::~InputRecorder()
{
    if (f_ && path_ != "") {
        this->commit();
        fclose(f_);
    }

    pm_.removeListener(pm_handle_);
}
