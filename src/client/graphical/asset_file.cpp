#include <algorithm>
#include <client/graphical/asset_file.hpp>
#include <client/graphical/exceptions.hpp>
#include <common/logger.hpp>
#include <string>

#include <config.h>

using namespace familyline::graphics;

std::string AssetItem::getItemOr(const char* key, const char* defaultval)
{
    auto it = this->items.find(std::string{key});
    if (it == this->items.end()) return std::string{defaultval};

    return it->second;
}

void AssetFile::loadFile(const char* ofile)
{
#ifdef _WIN32
    std::string sfile{ofile};

    // Replace slashes
    std::replace(sfile.begin(), sfile.end(), '/', '\\');
    const char* file = sfile.c_str();
#else
    const char* file = ofile;
#endif

    FILE* fAsset = fopen(file, "r");
    if (!fAsset) {
        char e[256 + 256];
        sprintf(e, "Failed to open asset file list %s", file);
        throw asset_exception(e, AssetError::AssetFileOpenError);
    }

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        fclose(fAsset);
        throw asset_exception(
            "Failed to initialize asset file parser", AssetError::AssetFileParserError);
    }

    auto& log = LoggerService::getLogger();

    yaml_parser_set_input_file(&parser, fAsset);

    log->write("asset-file-loader", LogType::Info, "loaded file %s", file);
    auto lassets = this->parseFile(&parser);
    log->write("asset-file-loader", LogType::Info, "loaded %zu assets", lassets.size());

    this->assets = this->processDependencies(std::move(lassets));

    yaml_parser_delete(&parser);
    fclose(fAsset);
}

std::vector<std::shared_ptr<AssetItem>> AssetFile::parseFile(yaml_parser_t* parser)
{
    auto& log = LoggerService::getLogger();

    std::vector<std::shared_ptr<AssetItem>> alist;

    bool asset_str  = false;
    bool asset_list = false;

    yaml_event_t event;

    // Try to find the 'assets' list
    do {
        if (!yaml_parser_parse(parser, &event)) {
            char s[256];
            sprintf(s, "Asset file parsing error: %d", parser->error);
            throw std::runtime_error(s);
        }

        if (event.type == YAML_SCALAR_EVENT) {
            if (!strcmp((const char*)event.data.scalar.value, "assets")) {
                asset_str = true;
            }
        } else if (asset_str && event.type == YAML_SEQUENCE_START_EVENT) {
            asset_list = true;
            break;

        } else {
            asset_str  = false;
            asset_list = false;
        }

    } while (event.type != YAML_STREAM_END_EVENT);

    if (!asset_str || !asset_list) {
        throw asset_exception(
            "Could not find the asset list in the file", AssetError::AssetNotFound);
    }

    AssetItem current_asset;
    bool is_key = false, is_val = false;
    std::string current_key;

    int list_sequences = 1;

    // Parse the list contents
    do {
        if (!yaml_parser_parse(parser, &event)) {
            char s[256];
            sprintf(s, "Asset list parsing error: %d", parser->error);
            throw std::runtime_error(s);
        }

        switch (event.type) {
                // The sequence start has already been processed before, so let's
                // direct into the mapping

            case YAML_MAPPING_START_EVENT:
                if (!asset_list) continue;

                current_asset = {};
                is_key        = true;
                break;

            case YAML_SCALAR_EVENT: {
                if (!asset_list) continue;

                const char* str = (const char*)event.data.scalar.value;

                // If is key, then save the key so we can remember later
                if (is_key) {
                    current_key = std::string{str};
                }

                // If is value, then save the old key+value in the file.
                if (is_val) {
                    if (current_key == "name") {
                        current_asset.name = str;
                    } else if (current_key == "type") {
                        current_asset.type = str;
                    } else if (current_key == "path") {
                        current_asset.path = str;
                    } else {
                        current_asset.items[current_key] = std::string{str};
                    }
                }

                is_key = !is_key;
                is_val = !is_val;
            } break;

            case YAML_MAPPING_END_EVENT: {
                if (!asset_list) continue;
                // Save the resulting asset in the alist

                auto replace_token = [](std::string& str, const char* from, const char* to) {
                    auto find_str = str.find(from);
                    if (find_str != std::string::npos)
                        str.replace(find_str, find_str + strlen(from), to);
                };

                std::string cpath = current_asset.path;
                replace_token(cpath, "${MODELS_DIR}/", MODELS_DIR);
                replace_token(cpath, "${MATERIALS_DIR}/", MATERIALS_DIR);
                replace_token(cpath, "${TEXTURES_DIR}/", TEXTURES_DIR);

                // todo: alert about confunding curly brackets with parenthesis!

                current_asset.path = cpath;

                log->write(
                    "asset-file-loader", LogType::Info, "found asset %s type %s path %s",
                    current_asset.name.c_str(), current_asset.type.c_str(),
                    current_asset.path.c_str());

                alist.push_back(std::shared_ptr<AssetItem>{new AssetItem{current_asset}});
                is_key = false;
                is_val = false;
            } break;

            // Handle unplanned lists correctly
            case YAML_SEQUENCE_START_EVENT:
                list_sequences++;
                break;

            case YAML_SEQUENCE_END_EVENT:
                list_sequences--;

                if (list_sequences <= 0) {
                    // End of sequence. End of file

                    asset_list = false;
                }

                break;

            default:
                is_key = false;
                is_val = false;
                break;

        }  // switch(...)

    } while (event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete(&event);

    return alist;
}

/* Process the assets and discover the dependencies between them */
std::vector<std::shared_ptr<AssetItem>> AssetFile::processDependencies(
    std::vector<std::shared_ptr<AssetItem>>&& assets)
{
    auto& log = LoggerService::getLogger();

    /* Callback for mesh dependency.
     * Return true if the mesh 'asset_name' is a child of mesh.
     *
     * If it's true, we need to load the childs before
     */
    auto fnMeshDep = [](const char* asset_name, AssetItem* const mesh) {
        auto meshtext = mesh->items.find("mesh.texture");
        if (meshtext != mesh->items.end()) {
            if (!strcmp(asset_name, meshtext->second.c_str())) return true;
        }

        meshtext = mesh->items.find("mesh.material");
        if (meshtext != mesh->items.end()) {
            if (!strcmp(asset_name, meshtext->second.c_str())) return true;
        }

        return false;
    };

    // TODO: Texture dependency.

    for (auto asset : assets) {
        auto fnDependency = [fnMeshDep, asset](std::shared_ptr<AssetItem> dependent) {
            if (asset->type == "mesh")
                return fnMeshDep(dependent->name.c_str(), asset.get());
            else
                return false;
        };

        asset->dependencies.resize(assets.size());
        auto it_dep =
            std::copy_if(assets.begin(), assets.end(), asset->dependencies.begin(), fnDependency);

        asset->dependencies.erase(it_dep, asset->dependencies.end());
        log->write(
            "asset-file-loader", LogType::Info, "asset %s has %zu dependencies",
            asset->name.c_str(), asset->dependencies.size());

        for (auto dep : asset->dependencies) {
            log->write("", LogType::Info, "\t%s", dep->name.c_str());
        }
    }

    return assets;
}

/* Get an asset */
AssetItem* AssetFile::getAsset(std::string_view name) const
{
    for (const auto asset : this->assets) {
        if (asset->name == name) {
            return asset.get();
        }
    }

    return nullptr;
}

std::optional<std::shared_ptr<AssetItem>> AssetFile::nextAsset()
{
    if ((unsigned)this->_asset_idx >= assets.size())
        return std::optional<std::shared_ptr<AssetItem>>();

    return make_optional(this->assets[this->_asset_idx++]);
}
