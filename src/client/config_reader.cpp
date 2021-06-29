#include <cerrno>
#include <client/config_reader.hpp>
#include <cstring>

extern "C" {
#include <yaml.h>
}

using namespace familyline;

std::vector<std::string> familyline::get_config_valid_paths()
{
    char file_settings[256] = {};

#if __has_include(<windows.h>) && __has_include(<winbase.h>)
    char* appdata = getenv("LOCALAPPDATA");

    if (appdata)
        snprintf(file_settings, 255, "%s\\Familyline\\settings.yaml", appdata);
    else
        strcpy(file_settings, "settings.yaml");

    return {
        "settings.yaml",
        file_settings,
    };

#else

    if (char* configdir = getenv("XDG_CONFIG_HOME"); configdir) {
        snprintf(file_settings, 255, "%s/familyline/settings.yaml", configdir);
    } else if (char* home = getenv("HOME"); home) {
        snprintf(file_settings, 255, "%s/.config/familyline/settings.yaml", home);
    } else if (char* logname = getenv("LOGNAME"); logname) {
        snprintf(file_settings, 255, "/home/%s/.config/familyline/settings.yaml", logname);
    } else {
        strcpy(file_settings, "/root/.config/familyline/settings.yaml");
    }

    return {"/etc/familyline/settings.yaml", file_settings};

#endif

    return {};
}

std::string replace_string(std::string str, std::string from, std::string to, size_t limit = 1)
{
    auto cidx = 0;
    auto fidx = str.find(from, cidx);

    auto i = 0;
    while (fidx != std::string::npos) {
        str.replace(fidx, from.size(), to);

        i++;
        if (i >= limit) break;

        cidx = fidx + to.size();
        fidx = str.find(from, cidx);
    }

    return str;
}

#if __has_include(<windows.h>) && __has_include(<winbase.h>)
#include <Shlobj.h>
#endif

/**
 * Expand path-specific keywords, like `~` and `%HOME%`, to
 * its full path, because they are usually not expanded
 */
std::string expand_path(std::string s)
{
#if __has_include(<windows.h>) && __has_include(<winbase.h>)
    WCHAR* path = nullptr;
    std::string homedir = "%HOME%";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &path))) {
        std::wstring ws(path);
        homedir = std::string(ws.begin(), ws.end());

        CoTaskMemFree(path);
    }

    char* appd = getenv("APPDATA");
    s          = replace_string(s, "%HOME%", homedir);
    s          = replace_string(s, "%APPDATA%", appd ? appd : "%APPDATA%");
    s          = replace_string(s, "\\", "/", MAX_PATH);

#else
    char* homedir = getenv("HOME");
    s             = replace_string(s, "~", homedir);

#endif
    return s;
}

void read_log_block_tags_section(yaml_parser_t* parser, ConfigData& data)
{
    int level = -1;

    do {
        yaml_token_t token;
        yaml_event_t event;

        yaml_parser_parse(parser, &event);

        switch (event.type) {
            case YAML_SEQUENCE_START_EVENT: level++; break;
            case YAML_SEQUENCE_END_EVENT: level--; break;
            case YAML_SCALAR_EVENT:
                printf("log/block_tags += %s\n", event.data.scalar.value);

                data.log.blockTags.push_back(std::string{
                    (char*)event.data.scalar.value,

                    event.data.scalar.length});
                break;
            default: continue;
        }

    } while (level >= 0);
}

/**
 * Read the log section of the config file
 *
 * The log section looks like this, more or less:
 *
 * ```yaml
 *
 * log:
 * block_tags:
 *   - player_manager
 *   - gl_renderer
 *
 * ```
 *
 */
void read_log_section(yaml_parser_t* parser, ConfigData& data)
{
    int level = -1;

    do {
        yaml_token_t token;
        yaml_event_t event;

        yaml_parser_parse(parser, &event);

        switch (event.type) {
            case YAML_MAPPING_START_EVENT: level++; break;
            case YAML_MAPPING_END_EVENT: level--; break;
            case YAML_SEQUENCE_START_EVENT: level++; break;
            case YAML_SEQUENCE_END_EVENT: level--; break;
            case YAML_SCALAR_EVENT: {
                std::string property((char*)event.data.scalar.value, event.data.scalar.length);

                if (property == "block_tags") {
                    read_log_block_tags_section(parser, data);
                }
                break;
            }
            default: continue;
        }

    } while (level >= 0);
}


/**
 * Read the player section of the config file
 *
 * The log section looks like this, more or less:
 *
 * ```yaml
 *
 * player:
 *   username: "Arthur"
 *
 * ```
 *
 */
void read_player_section(yaml_parser_t* parser, ConfigData& data)
{
    int level = -1;
    std::string current_property = "";

    do {
        yaml_token_t token;
        yaml_event_t event;

        yaml_parser_parse(parser, &event);
        
        switch (event.type) {
            case YAML_MAPPING_START_EVENT: level++; break;
            case YAML_MAPPING_END_EVENT: level--; break;
            case YAML_SEQUENCE_START_EVENT: level++; break;
            case YAML_SEQUENCE_END_EVENT: level--; break;
            case YAML_SCALAR_EVENT: {
                std::string value((char*)event.data.scalar.value, event.data.scalar.length);
                
                if (current_property == "") {
                    current_property = value;
                    break;
                }

                if (current_property == "username") {
                    data.player.username = value;
                    printf("player.username = %s\n", value.c_str());
                    current_property = "";
                }
                break;
            }
            default: continue;
        }

    } while (level >= 0);
}

bool familyline::read_config_from(std::string_view path, ConfigData& data)
{
    FILE* fConfig = fopen(path.data(), "r");
    if (!fConfig) {
        /// Do not finding a config file is not abnormal, do not report
        if (errno != 0) {
            fprintf(
                stderr, "could not open config file '%s', error %d (%s)\n", path.data(), errno,
                strerror(errno));
        }

        return false;
    }

    fprintf(stderr, "opening file %s\n", path.data());

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        fputs("Failed to initialize parser!\n", stderr);
        return false;
    }

    yaml_parser_set_input_file(&parser, fConfig);

    int level = 0;

    std::string currentProperty = "";

    do {
        yaml_event_t event;

        /**
         * Parsing events instead of tokens gives us the ability to differentiate
         * an end of a list from an end of a map, for example.
         * But, for some reason, we lose the ability to differentiate between
         * a key and a value.
         *
         * Since we would not need a lot more of code for us to make this difference
         * of key and value, it was chosen to use the event-based parsing
         */
        if (!yaml_parser_parse(&parser, &event)) {
            printf("Parser error %d\n", parser.error);
            return false;
        }

        switch (event.type) {
            case YAML_DOCUMENT_START_EVENT: level = 0; break;
            case YAML_DOCUMENT_END_EVENT: level = -1; break;
            case YAML_SEQUENCE_START_EVENT: level++; break;
            case YAML_SEQUENCE_END_EVENT: level--; break;
            case YAML_SCALAR_EVENT: {
                std::string value((char*)event.data.scalar.value, event.data.scalar.length);

                // a property was expected
                if (currentProperty == "") {
                    if (value == "log") {
                        read_log_section(&parser, data);                        
                    } else if (value == "player") {
                        read_player_section(&parser, data);
                    } else {
                        currentProperty = value;
                    }

                } else {
                    // a value was expected

                    if (currentProperty == "enable_input_recording") {
                        printf("enableInputRecording = true\n");
                        data.enableInputRecording = (value == "true");
                    } else if (currentProperty == "default_input_record_directory") {
                        printf("defaultInputRecordDir = %s\n", expand_path(value).c_str());
                        data.defaultInputRecordDir = expand_path(value);
                        ;
                    }

                    currentProperty = "";
                }

                break;
            }
                // The mapping event seems to come after a key/value pair,
                // or after a list
            case YAML_MAPPING_END_EVENT: currentProperty = ""; break;
            default: continue;
        }

    } while (level >= 0);

    yaml_parser_delete(&parser);
    fclose(fConfig);

    return true;
}
