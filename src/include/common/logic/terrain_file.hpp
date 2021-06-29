#pragma once

/**
 * Terrain file read class
 *
 * Copyright (C) 2020 Arthur Mendes
 */

#include <cstdint>
#include <optional>
#include <string_view>
#include <string>
#include <tuple>
#include <vector>

namespace familyline::logic
{
/**
 * The terrain file header
 */
struct TerrainFileHeader {
    uint32_t magic;
    uint32_t file_crc32;
    uint32_t file_version;
    uint32_t terrain_off;
};

struct TerrainHeader {
    uint32_t magic;
    uint32_t width, height;
    uint32_t terrain_crc32;
    uint32_t name_off, author_off, desc_off;
    uint32_t terrain_data_off;
    uint32_t terrain_type_off;
};

class TerrainFile
{
private:
    FILE* fTerrain_;
    uint32_t file_magic    = 0x45544c46;  // 'FLTE'
    uint32_t terrain_magic = 0x45454554;  // 'TEEE'

    std::vector<uint16_t> height_data;
    std::vector<uint16_t> type_data;

    std::string name_;
    std::string description_;
    std::vector<std::string> authors_;
    std::tuple<uint32_t, uint32_t> size_;

    /**
     * Read the file header, return the offset to the terrain header.
     *
     * If the offset could not be read, return 0, since 0 will never be a
     * valid terrain header offset.
     */
    uint32_t readFileHeader(FILE* f);

    /**
     * Read the terrain header, return the TerrainHeader structure
     *
     */
    std::optional<TerrainHeader> readTerrainHeader(FILE* f, uint32_t header_off);

    bool checkCRC(uint32_t expectedCRC, FILE* f, uint32_t start, uint32_t crc_field_off);

    /**
     * See if the CRC file match with the one you pass as a parameter
     */
    bool checkFileCRC(uint32_t expectedCRC, FILE* f);

    /**
     * See if the CRC terrain match with the one you pass as a parameter
     *
     * Note that this function will work correctly only if the terrain header comes before
     * of everything but the terrain file header. This will be fixed sometime, but
     * it is simpler for now to do this way.
     */
    bool checkTerrainCRC(uint32_t expectedCRC, FILE* f, uint32_t header_off);

    /**
     * Read the terrain height and type data.
     *
     * Return true if it could, false if it could not
     */
    bool readTerrainData(FILE* f, TerrainHeader& th);

    /**
     * Read a pascal string (a string prefixed by its length) from the file,
     * in the current position
     *
     * The name and each one of the authors are stored this way
     */
    std::string readPascalString(FILE* f);

    std::string readName(FILE* f, const TerrainHeader& th);
    std::string readDescription(FILE* f, const TerrainHeader& th);
    std::vector<std::string> readAuthors(FILE* f, const TerrainHeader& th);

public:

    TerrainFile()
        {}
    
    /**
     * Create an empty terrain file
     */
    TerrainFile(size_t w, size_t h)
        : size_(std::make_tuple(w, h)),
          height_data(std::vector<uint16_t>(w * h, 0)),
          type_data(std::vector<uint16_t>(w * h, 0))
    {
    }

    /**
     * Create an empty terrain file with a custom height data
     *
     * Useful for testing
     */
    TerrainFile(size_t w, size_t h, std::vector<uint16_t> height)
        : size_(std::make_tuple(w, h)),
          height_data(height),
          type_data(std::vector<uint16_t>(w * h, 0))
    {}

    /**
     * Open the terrain
     *
     * Return true if no error occurred, false if an error occurred.
     */
    bool open(std::string_view path);

    std::string_view getName() const { return name_; }
    std::string_view getDescription() const { return description_; }
    const std::vector<std::string>& getAuthors() const { return authors_; }
    std::tuple<uint32_t, uint32_t> getSize() const { return size_; }

    const std::vector<uint16_t>& getHeightData() { return height_data; }
    const std::vector<uint16_t>& getTypeData() { return type_data; }
};
}  // namespace familyline::logic
