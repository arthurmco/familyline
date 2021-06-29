#pragma once

/**
 * Reads the configuration file.
 *
 * We will use the UNIX principle: the configuration files can be in more
 * than one place:
 *   On Windows, is inside of the app directory and inside of the AppData directory
 *   On Unixes, can be in /etc/familyline, or in ~/.config/familyline.
 *
 * See the settings.yaml in the repository root for an example of the configuration file,
 * with all settings specified.
 *
 * The latter places can override the configurations in the earlier places.
 */

#include <string>
#include <vector>

namespace familyline
{
struct ConfigData {
    struct {
        /**
         * Do not allow the specified log tags to be shown in
         * the logs.
         *
         * Useful only for debugging, when you want to debug issues in some subsystem,
         * and some other subsystem keeps logging a lot of data you do not want to
         * see
         */
        std::vector<std::string> blockTags;
    } log;

    /**
     * Enable input recording for the games you want to play
     *
     * This does not mean record the screen, only the player inputs, so that someone
     * else can reproduce the game.
     */
    bool enableInputRecording = false;

    /**
     * The directory where recorded games will be saved
     */
    std::string defaultInputRecordDir = ".";

    struct {
        std::string username = "DefaultUser";
    } player;
};

    std::vector<std::string> get_config_valid_paths();

    /**
     * Read configuration data from a specific path
     *
     * Note that it will only replace the fields it knows, and the ones
     * you specify
     *
     * Return true if it could read the file, false if it couldn't
     */
    bool read_config_from(std::string_view path, ConfigData& data);
    
}  // namespace familyline
