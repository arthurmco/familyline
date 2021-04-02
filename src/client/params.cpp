#include <fmt/core.h>
#include <client/params.hpp>
#include <cstdlib>

#include <client/graphical/opengl/gl_device.hpp>
#include "client/graphical/device.hpp"

static void show_help()
{
    fmt::print("Familyline command line help:\n");
    fmt::print("  --version:\n\tPrint version and, if compiled inside a Git repo, commit hash\n\n");
    fmt::print("  --help:\n\tPrint this help information\n\n");
    fmt::print("  --size <W>x<H>:\n\tChanges the game resolution to <W>x<H> pixels\n\n");
    fmt::print("  --file <path>:\n\tLoad a map from the specified path\n\n");
    fmt::print("  --readinput <path>:\n\tLoad an input file. The game will start in the map\n");
    fmt::print("  \tyou played when you recorded\n\n");
    fmt::print("  --connect <addr>[:<port>]:\n\tConnect to the game server specified as <addr>\n\n");
    fmt::print(
        "  --log [<filename>|screen]:\n\tLogs to filename 'filename', or screen to log to screen, or\n"
        "  \twherever stderr is bound to\n\n");
}


static void show_version()
{
    fmt::print("Familyline " VERSION "\n");
    fmt::print("Compiled in " __DATE__ "\n");
    fmt::print("Commit hash " COMMIT "\n");
    fmt::print("Running on {}", SDL_GetPlatform());
    fmt::print("\n");
}

std::tuple<int, int> get_size(std::string_view s) {
    int width=-1, height=-1;

    if (sscanf(s.data(), "%dx%d", &width, &height) <= 1) {
        return std::tuple<int, int>(-1, -1);
    }

    return std::tie(width, height);
}

FILE* get_log_device(std::string_view s) {
    if (s == "stdout") {
        return stdout;
    }

    if (s == "stderr") {
        return stderr;
    }

    FILE* f = fopen(s.data(), "w");
    if (!f) {
        fmt::print("could not open log file '{}': \n", s);
        fmt::print("{} (error {})\n\n", strerror(errno), errno);
        return nullptr;
    }

    return f;
}

std::vector<std::string> get_available_renderers() {
    std::vector<std::string> rs;

    #ifdef RENDERER_OPENGL
    rs.push_back("opengl");
    #endif

    return rs;
}

std::vector<std::unique_ptr<familyline::graphics::Device>> get_device_list(std::string_view renderer) {
    auto rs = get_available_renderers();

    if (rs.empty()) {
        fmt::print("no renderers available!\n");
        fmt::print("You probably did not compile the game right!\n");
        std::exit(1);
    }

    if (std::find(rs.begin(), rs.end(), renderer) == rs.end()) {
        fmt::print("renderer {} is invalid!\n", renderer);
        fmt::print("Valid renderers are: ");
        for (auto& r : rs) {
            fmt::print("{}", r);
        }
        fmt::print("\n");
        std::exit(1);
    }

#ifdef RENDERER_OPENGL
    if (renderer == "opengl") {
        return familyline::graphics::GLDevice::getDeviceList();
    }
#endif

    return std::vector<std::unique_ptr<familyline::graphics::Device>>{};
}

ParamInfo parse_params(const std::vector<std::string>& params)
{
    ParamInfo pi;
    bool next_is_size = false;
    bool next_is_log = false;
    bool next_is_file = false;
    bool next_is_input = false;
    bool next_is_server = false;
    
    for (auto& p : params) {
        ////// parse values
        if (next_is_size) {
            std::tie(pi.width, pi.height) = get_size(p);
            next_is_size = false;
            continue;
        }

        if (next_is_log) {
            pi.log_device = get_log_device(p);
            next_is_log = false;
            continue;
        }


        if (next_is_file) {
            pi.mapFile = p;
            next_is_file = false;
            continue;
        }

        if (next_is_input) {
            pi.inputFile = p;
            next_is_input = false;
            continue;
        }


        if (next_is_server) {
            pi.serverAddress = p;
            next_is_server = false;
            continue;
        }


        ////// parse params

        if (p == "--help") {
            show_help();
            exit(0);
        }

        if (p == "--version") {
            show_version();
            exit(0);
        }

        if (p == "--size") {
            next_is_size = true;
            continue;
        }

        if (p == "--file") {
            next_is_file = true;
            continue;
        }

        if (p == "--readinput") {
            next_is_input = true;
            continue;
        }

        if (p == "--log") {
            next_is_log = true;
            continue;
        }

        if (p == "--connect") {
            next_is_server = true;
            continue;
        }


        fmt::print("\t param: {}\n", p);
    }

    /* Treat errors that happen when we expect a value that did not exist */

    if (next_is_size) {
        fmt::print("Expected size in format <width>x<height>\n");
        exit(1);
    }

    if (next_is_log || !pi.log_device) {
        fmt::print("Expected log device: can be stdout, stderr or a filename\n");
        exit(1);
    }

    if (next_is_file) {
        fmt::print("Expected a map file name\n");
        exit(1);
    }

    if (next_is_input) {
        fmt::print("Expected an input record file (the file with .frec extension)\n");
        exit(1);
    }

    pi.devices = get_device_list(pi.renderer);
    
    return pi;
}
