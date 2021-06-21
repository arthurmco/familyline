
/***
    Familyline main file

    Copyright 2016, 2017, 2019-2020 Arthur Mendes.

***/
#include <config.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <client/familyline.hpp>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>  //glm::lookAt()
#include <iterator>
#include <memory>
#include <thread>

#include "common/net/network_player.hpp"

using namespace familyline;
using namespace familyline::logic;
using namespace familyline::graphics;
using namespace familyline::graphics::gui;
using namespace familyline::input;
using namespace familyline::net;

// TODO: create a shader manager *and* a texture manager

/**
 * Initialize global sockets.
 *
 * Currently only meaningful on Windows because of that WSADATA thing
 *
 * Yes, this will be moved out of here.
 */
bool init_network()
{
#ifdef WIN32
    WSADATA trash;
    if (WSAStartup(MAKEWORD(2, 0), &trash) != 0) return false;
#endif

    return true;
}

void end_network()
{
#ifdef WIN32
    WSACleanup();
#endif
}

void run_game_loop(LoopRunner& lr, int& framecount)
{
    while (true) {
        if (!lr.run()) {
            break;
        }

        framecount++;
    }
}

/**
 * This function creates a game object, that will represents the state of a game
 *
 * We pass some interface information, a loop runner, parameter data (the StartGameinfo),
 * configuration data, and a function that returns a session, i.e, the objects for players
 * and teams (we call them colonies)
 *
 * Since multiplayer and singleplayer have different player objects, we need this callback
 * to create them
 */
template <typename SessionF>
Game* start_game(
    GraphicalInfo ginfo, LoopRunner& lr, const StartGameInfo& sgai, ConfigData& confdata,
    SessionF fnSession);

/**
 * Wait for a list futures promises, then return
 *
 * Return true when every item on the promise resolves
 *
 * The FnValidator is a function that you should call, and the cancel is a variable
 * you should set when you want to cancel the operation
 */
template <typename Result, typename Expect, std::invocable<Expect&> FnValidator>
bool waitFutures(
    std::vector<std::pair<Result, std::future<Expect>>>& promises, std::vector<Result>& result,
    FnValidator&& validator, auto& cancel)
{
    bool all_loaded = false;
    while (!all_loaded) {
        if (cancel) break;

        for (auto& [id, promise] : promises) {
            if (!promise.valid()) continue;

            //            printf("waiting on %zu\n", id);

            if (auto fstatus = promise.wait_for(std::chrono::milliseconds(100));
                fstatus == std::future_status::ready) {
                printf("%zu is ready\n", id);

                auto res = promise.get();
                if (!res.has_value()) {
                    printf("\tan error occurred while handling message for %lu\n", id);
                    continue;
                }

                if (validator(res)) {
                    result.push_back(id);
                }
            }

            if (result.size() == promises.size()) {
                all_loaded = true;
            }
        }
    }

    return all_loaded;
}

int start_networked_game(
    GraphicalInfo& ginfo, GamePacketServer& gps, std::vector<NetworkClient>& clients,
    ConfigData& cdata)
{
    printf("Loading the game...\n");
    printf("\tWaiting for the other clients to load\n");
    assert(clients.size() > 0);

    bool all_loaded            = false;
    bool all_ready             = false;
    std::atomic<bool> quitting = false;

    std::thread netthread([&]() {
        int iters = 0;
        while (!quitting) {
            iters++;

            gps.update();
            std::for_each(clients.begin(), clients.end(), [&](NetworkClient& c) { c.update(); });
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // 1 min timeout
            if (iters == 1000 * 30) {
                puts("TIMED OUT");
                break;
            }
        }
    });

    std::vector<std::pair<uint64_t, std::future<tl::expected<bool, NetResult>>>> loading_promises;
    std::transform(
        clients.begin(), clients.end(), std::back_inserter(loading_promises),
        [](NetworkClient& c) { return std::make_pair(c.id(), c.waitLoading()); });
    std::vector<uint64_t> loading_clients;

    gps.sendLoadingMessage(0);
    /// TODO: load the map, assets, game class...

    /// TODO: fix an issue where two messages can come on the same TCP packet.
    /// This might happen here.
    /// Fix here and on Rust server, but only when receiving.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    gps.sendLoadingMessage(100);
    printf("\t our game loaded! \n");

    all_loaded = waitFutures(
        loading_promises, loading_clients, [&](auto& res) { return res.value_or(false) == true; },
        quitting);

    if (!all_loaded) {
        quitting = true;
        printf("\tanother client failed to load, exiting\n");
        netthread.join();
        return 1;
    }

    printf("\tnotifying other clients we are about to start, and waiting them\n");
    std::vector<std::pair<uint64_t, std::future<tl::expected<bool, NetResult>>>> start_promises;
    std::transform(
        clients.begin(), clients.end(), std::back_inserter(start_promises),
        [](NetworkClient& c) { return std::make_pair(c.id(), c.waitReadyToStart()); });
    std::vector<uint64_t> start_clients;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    gps.sendStartMessage();

    all_ready = waitFutures(
        start_promises, start_clients, [&](auto& res) { return res.value_or(false) == true; },
        quitting);

    if (!all_ready) {
        quitting = true;
        printf("\tanother client failed to start the game, exiting\n");
        netthread.join();
        return 1;
    }

    printf("\tstarting the game\n");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    quitting = true;
    netthread.join();

    LoopRunner lr;

    int frames = 0;

    auto createMultiplayerSession_fn = [&](logic::Terrain& map, auto& local_player_info) {
        std::map<unsigned int /*player_id*/, std::reference_wrapper<logic::Colony>> player_colony;

        auto pm = std::make_unique<PlayerManager>();
        for (auto& c : clients) {
            pm->add(std::make_unique<NetworkPlayer>(*pm.get(), map, c), false);
        }
        auto hid = pm->add(
            std::unique_ptr<Player>(new HumanPlayer{*pm.get(), map, local_player_info.name.c_str(), 0, true}));
        local_player_info.id = hid;
        printf("\n%d\n", hid);

        auto cm = std::make_unique<ColonyManager>();
        for (auto& c : clients) {
            auto* player = *pm->get(c.id());
            int color    = (int)player->getCode() & 0x00ffffff;

            auto& alliance = cm->createAlliance("alliance");
            auto& colony   = cm->createColony(
                *player, color, std::optional<std::reference_wrapper<Alliance>>{alliance});
            
            player_colony.emplace(c.id(), std::reference_wrapper(colony));
        }

        {
            auto* player = *pm->get(hid);
            int color    = (int)player->getCode() & 0x00ffffff;

            auto& alliance = cm->createAlliance("alliance");
            auto& colony   = cm->createColony(
                *player, color, std::optional<std::reference_wrapper<Alliance>>{alliance});
            
            player_colony.emplace(hid, std::reference_wrapper(colony));
        }
        
        return PlayerSession{std::move(pm), std::move(cm), player_colony};
    };

    Game* g = start_game(
        ginfo, lr, StartGameInfo{ASSET_FILE_DIR "terrain_test.flte", std::nullopt}, cdata,
        createMultiplayerSession_fn);

    lr.load([&]() { return g->runLoop(); });
    run_game_loop(lr, frames);

    return 0;
}

void start_networked_game_room(
    GraphicalInfo& ginfo, CServer& cserv, std::function<void(std::string, NetResult)> errHandler,
    ConfigData& cdata)
{
    CServerInfo si = {};
    auto ret       = cserv.getServerInfo(si);
    if (ret != NetResult::OK) {
        errHandler("Failure to get server info", ret);
        return;
    }

    std::optional<GamePacketServer> gps;

    bool exit = false;
    while (!exit) {
        fmt::print("\033[2J\n\033[1;1H");
        fmt::print("\033[1m{}\033[0m\t ({})\n", si.name, cserv.getAddress());
        fmt::print("\t\033[32m{}\033[0m/{} clients\n", si.clients.size(), si.max_clients);
        fmt::print("\n---------------------------------------------------\n");

        for (auto& cli : si.clients) {
            std::string readycolor = cli.ready ? "\033[34m" : "";
            fmt::print(
                " {} {}{}\033[0m\n", cli.id == cserv.getUserID() ? "=>" : "  ", readycolor,
                cli.name);
        }

        fmt::print("\033[{}B", si.max_clients - si.clients.size() + 1);
        fmt::print("\033[3B [(u)pdate, (q)uit, (c)onnect, (t)oggle ready]> ");

        fflush(stdin);
        auto v = getchar();

        switch (v) {
            case 'q':
            case 'Q': exit = true; break;
            case 'u':
            case 'U':
                ret = cserv.getServerInfo(si);
                if (ret != NetResult::OK) {
                    errHandler("Failure to get server info", ret);
                    return;
                }
                break;
            case 't':
            case 'T':
                ret = cserv.toggleReady(!cserv.isReady());
                if (ret != NetResult::OK) {
                    errHandler("Failure to set ready", ret);
                    return;
                }

                ret = cserv.getServerInfo(si);
                if (ret != NetResult::OK) {
                    errHandler("Failure to get server info", ret);
                    return;
                }

                if (std::all_of(
                        si.clients.begin(), si.clients.end(),
                        [](auto& v) { return v.ready == true; }) &&
                    si.clients.size() >= 2) {
                    cserv.connect();
                }

                break;
            case 'c':
            case 'C':
                ret = cserv.getServerInfo(si);
                if (ret != NetResult::OK) {
                    errHandler("Failure to get server info", ret);
                    return;
                }

                ret = cserv.connect();
                if (ret != NetResult::OK) {
                    errHandler("Failure to get the game server info", ret);

                } else {
                    printf("connecting to the game server\n");
                    gps = cserv.getGameServer();
                    if (!gps) {
                        printf("error: connection failed");
                        continue;
                    }

                    if (!gps->connect()) {
                        printf("error: failed to connect to server");
                        continue;
                    }

                    std::vector<NetworkClient> clients;
                    auto clientfut = gps->waitForClientConnection();
                    int cycles     = 0;
                    for (;;) {
                        gps->update();

                        auto fstatus = clientfut.wait_for(std::chrono::milliseconds(1));
                        if (fstatus == std::future_status::ready) {
                            clients = clientfut.get()
                                          .map_error([](auto e) {
                                              switch (e) {
                                                  case NetResult::ConnectionTimeout:
                                                      printf(
                                                          "Timed out while waiting other players. "
                                                          "Please try again.\n");
                                                      break;

                                                  case NetResult::UnexpectedDisconnect:
                                                      printf(
                                                          "The client disconnected, but it should "
                                                          "not disconnect!");
                                                      break;

                                                  case NetResult::ServerError:
                                                      printf("Unknown server error.\n");
                                                      break;

                                                  default: printf("Unexpected error\n");
                                              }

                                              return e;
                                          })
                                          .value_or(std::vector<NetworkClient>{});
                            break;
                        }
                    }

                    if (clients.size() <= 0) {
                        printf("No clients.\n");
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                        continue;
                    }

                    start_networked_game(ginfo, *gps, clients, cdata);
                    exit = true;
                }

                fflush(stdout);
                sleep(3);
                break;
        }
    }
}

/**
 * Enable console color on Windows 8 or newer
 *
 * On other operating systems, this is a noop
 */
void enable_console_color(FILE* f)
{
#ifdef _WIN32
    if (f == stdout || f == stderr) {
        // Set output mode to handle virtual terminal sequences, so the
        // color escape sequences can work on Windows
        HANDLE hOut = GetStdHandle(f == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) {
            GetLastError();
        }

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) {
            GetLastError();
        }

        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, dwMode)) {
            GetLastError();
        }
    }
#endif
}

#ifdef _WIN32
std::string get_windows_version(const OSVERSIONINFOEX& osvi)
{
    if (osvi.dwMajorVersion < 6) {
        return fmt::format("version {}.{} (Unsupported)", osvi.dwMajorVersion, osvi.dwMinorVersion);
    }

    std::string version;
    if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 &&
        osvi.wProductType == VER_NT_WORKSTATION)
        version = "Windows Vista";
    else if (
        osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 &&
        osvi.wProductType != VER_NT_WORKSTATION)
        version = "Windows Server 2008";
    else if (
        osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 &&
        osvi.wProductType == VER_NT_WORKSTATION)
        version = "Windows 7";
    else if (
        osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 &&
        osvi.wProductType != VER_NT_WORKSTATION)
        version = "Windows Server 2008 R2";
    else if (
        osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2 &&
        osvi.wProductType == VER_NT_WORKSTATION)
        version = "Windows 8";
    else if (
        osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2 &&
        osvi.wProductType != VER_NT_WORKSTATION)
        version = "Windows Server 2012";
    else if (
        osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3 &&
        osvi.wProductType == VER_NT_WORKSTATION)
        version = "Windows 8.1";
    else if (
        osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3 &&
        osvi.wProductType != VER_NT_WORKSTATION)
        version = "Windows Server 2012 R2";
    else if (
        osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 &&
        osvi.wProductType == VER_NT_WORKSTATION)
        version = "Windows 10";
    else if (
        osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 &&
        osvi.wProductType != VER_NT_WORKSTATION)
        version = "Windows Server 2016";
    else
        version = fmt::format("Windows {}.{}", osvi.dwMajorVersion, osvi.dwMinorVersion);

    return fmt::format("{} {}", version, osvi.szCSDVersion);
}
#endif

/**
 * Get current system name
 *
 * The first string of the tuple returns the system name, in a more basic way
 * The second returns the version
 * The third returns more information:
 *    - On Linux, it returns the distro name and version
 *    - On Windows, it returns the kernel version (like 2004)
 *
 * This is only useful to diagnostics, and to do telemetry in the future
 */
std::tuple<std::string, std::string, std::string> get_system_name()
{
    std::string sysname = std::string{SDL_GetPlatform()};

#ifdef _WIN32

    /* Microsoft wants to get rid of GetVersionEx because other people
     * use them to determine features
     *
     * I only need to get the current version to show to people, so please, have mercy :)
     *
     * YES, I know it is deprecated.
     */
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    GetVersionEx((LPOSVERSIONINFO)&osvi);

    std::string version = get_windows_version(osvi);
    std::string sysinfo = fmt::format("Build {}", osvi.dwBuildNumber);
#else
    // all others probably are unix
    std::string version;
    std::string sysinfo;

    struct utsname uts;
    if (uname(&uts) == 0) {
        version = fmt::format("{} {}", uts.sysname, uts.release);
        sysinfo = fmt::format("{}", uts.version);
    }

#endif

    return std::tie(sysname, version, sysinfo);
}

static int show_starting_menu(
    const ParamInfo& pi, GraphicalInfo ginfo, LoopRunner& lr, ConfigData& confdata);

template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

template <typename SessionF>
Game* start_game(
    GraphicalInfo ginfo, LoopRunner& lr, const StartGameInfo& sgai, ConfigData& confdata,
    SessionF fnSession)
{
    GFXGameInit gi{
        ginfo.win,
        ginfo.f3D,
        ginfo.fGUI,
        ginfo.guir,
    };

    auto& log = LoggerService::getLogger();

    std::string mapfile = sgai.mapFile;
    std::unique_ptr<InputReproducer> irepr;

    if (sgai.inputFile) {
        irepr = std::make_unique<InputReproducer>(*sgai.inputFile);

        if (!irepr->open()) {
            mapfile = ASSET_FILE_DIR "terrain_test.flte";
            irepr.reset();
            ginfo.win->showMessageBox(
                "Familyline error", SysMessageBoxFlags::Error,
                fmt::format(
                    "Could not open input record {}\n"
                    "Check the logs for more information.",
                    *sgai.inputFile));
            return nullptr;

        } else {
            mapfile = irepr->getTerrainFile();
        }
    }

    Game* g   = new Game(gi);
    auto& map = g->initMap(mapfile);

    std::string player_name = confdata.player.username;
    auto pinfo              = InitPlayerInfo{player_name, uint64_t(-1)};

    std::unique_ptr<PlayerManager> pm;
    std::unique_ptr<ColonyManager> cm;
    uint64_t human_player_id = 0;
    PlayerSession session;
    g->initAssets();
    ObjectFactory* of = g->initObjectFactory();

    if (irepr) {
        if (!irepr->verifyObjectChecksums(of)) {
            irepr.reset();
            ginfo.win->showMessageBox(
                "Familyline error", SysMessageBoxFlags::Error,
                fmt::format(
                    "Could not open input record {}, objects do not match\n"
                    "The person who recorded might have a mod you do not have, or a"
                    "different version than you.",
                    *sgai.inputFile));
            return nullptr;

        } else {
            /// loading input from a file
            session = irepr->createPlayerSession(map);
            pm      = std::move(session.players);
            cm      = std::move(session.colonies);

            human_player_id = pm->add(std::unique_ptr<Player>(
                new HumanPlayer{*pm.get(), map, player_name.c_str(), 0, false}));
            auto* player    = *(pm->get(human_player_id));
            auto& alliance  = cm->createAlliance(std::string{player->getName()});
            auto& colony    = cm->createColony(
                *player, 0xffffff, std::optional<std::reference_wrapper<Alliance>>{alliance});
            session.player_colony.emplace(human_player_id, std::reference_wrapper(colony));
        }
    } else {
        session         = fnSession(map, pinfo);
        pm              = std::move(session.players);
        cm              = std::move(session.colonies);
        human_player_id = pinfo.id;
    }

    if (human_player_id == -1) {
        throw std::runtime_error{"Could not create the human player"};
    }

    // You can't reproduce a record while recording.
    //
    // It will work, will not overwrite any file (because the filename is decided by the
    // recording date), but it is redundant.
    if (confdata.enableInputRecording && !irepr) {
        log->write("game", LogType::Info, "This game inputs will be recorded");

        auto ir = std::make_unique<InputRecorder>(*pm.get());

        auto rawtime        = time(NULL);
        auto ftime          = localtime(&rawtime);
        auto recordfilename = fmt::format(
            "record-{}{:04d}{:02d}-{:02d}{:02d}{:02d}.frec", 1900 + ftime->tm_year, ftime->tm_mon,
            ftime->tm_mday, ftime->tm_hour, ftime->tm_min, ftime->tm_sec);
        std::filesystem::path recordpath(confdata.defaultInputRecordDir);
        recordpath.make_preferred() /= recordfilename;

        log->write("game", LogType::Info, "\trecord destination: %s", recordpath.c_str());

        if (!ir->createFile(recordpath.string(), of)) {
            log->write("game", LogType::Error, "\tinput record file could not be created");
        } else {
            g->initRecorder(std::move(ir));
        }
    }

    if (irepr) {
        log->write(
            "game", LogType::Info, "Replaying inputs from file %s", (*sgai.inputFile).c_str());

        irepr->reset();

        // dispatch 10 seconds worth of events
        irepr->dispatchEvents((1000 / LOGIC_DELTA) * 1);
        g->initReproducer(std::move(irepr));
    }

    g->initPlayers(std::move(pm), std::move(cm), session.player_colony, human_player_id);
    g->initObjectManager();
    g->initLoopData(human_player_id);

    return g;
}

void start_networked_game_cmdline(GraphicalInfo ginfo, std::string addr, ConfigData& cdata)
{
    auto treat_errors = [&](std::string msg, NetResult ret) {
        switch (ret) {
            case NetResult::ConnectionError:
                fmt::print("{}: {}\n", msg, fmt::format("Could not connect to address {}", addr));
                break;
            case NetResult::WrongPassword:
                fmt::print("{}: {}", fmt::format("Server {} has a password", addr));
                break;
            case NetResult::LoginFailure:
                fmt::print(
                    "{}: {}\n", msg,
                    fmt::format(
                        "Could not log in to {}\nThe server was found, but we could not "
                        "login there.",
                        addr));
                break;
            case NetResult::ConnectionTimeout:
                fmt::print(
                    "{}: {}\n", msg,
                    fmt::format(
                        "Timed out while connecting to {}\nThe server probably does not "
                        "exist, or there is a firewall issue.",
                        addr));
                break;
            case NetResult::ServerError:
                fmt::print(
                    "{}: {}\n", msg,
                    fmt::format(
                        "Error while connecting to {}. The server sent incorrect data to "
                        "the client.",
                        addr));
                break;
            case NetResult::NotAllClientsConnected:
                fmt::print(
                    "{}: {}\n", msg,
                    fmt::format(
                        "Error where connecting to {}: The server reported that not all clients "
                        "were connected",
                        addr));
                break;
            default:
                fmt::print(
                    "{}: {}\n", msg,
                    fmt::format("Could not connect to address {}: unknown error {}", addr, ret));
        }
    };

    CServer cserv{};
    auto ret = cserv.login(addr, cdata.player.username);
    if (ret != NetResult::OK) {
        treat_errors("Connection error", ret);
        return;
    }

    start_networked_game_room(ginfo, cserv, treat_errors, cdata);

    cserv.logout();
}

/////////
/////////
/////////

ConfigData read_settings()
{
    ConfigData confdata;

    if (auto confenv = getenv("CONFIG_FILE_PATH"); confenv) {
        fprintf(stderr, "reading config file from envvar (%s)\n", confenv);
        read_config_from(confenv, confdata);
        return confdata;
    }

    auto confpaths = get_config_valid_paths();
    for (auto& confpath : confpaths) {
        read_config_from(confpath, confdata);
    }

    return confdata;
}

int main(int argc, char const* argv[])
{
    auto pi = parse_params(std::vector<std::string>{argv + 1, argv + argc});
    enable_console_color(pi.log_device);

    fmt::print("Using resolution {} x {} \n", pi.width, pi.height);
    ConfigData confdata = read_settings();

    LoggerService::createLogger(pi.log_device, LogType::Debug, confdata.log.blockTags);

    auto& log = LoggerService::getLogger();

    auto [sysname, sysversion, sysinfo] = get_system_name();

    log->write("", LogType::Info, "Familyline " VERSION);
    log->write("", LogType::Info, "built on " __DATE__ " by " USERNAME);
#if defined(COMMIT)
    log->write("", LogType::Info, "git commit is " COMMIT);
#endif
    log->write("", LogType::Info, "Running on OS %s", sysname.c_str());
    log->write("", LogType::Info, "  version: %s (%s)", sysversion.c_str(), sysinfo.c_str());

    log->write("", LogType::Info, "Using renderer '%s'", pi.renderer.c_str());

    char timestr[32];

    auto tm     = time(NULL);
    auto tminfo = localtime(&tm);
    strftime(timestr, 32, "%F %T", tminfo);
    log->write("", LogType::Info, "Actual date is %s", timestr);

    log->write("", LogType::Info, "Default model directory is " MODELS_DIR);
    log->write("", LogType::Info, "Default texture directory is " TEXTURES_DIR);
    log->write("", LogType::Info, "Default material directory is " MATERIALS_DIR);

    LoopRunner lr;

    graphics::Window* win = nullptr;
    GUIManager* guir      = nullptr;
    PlayerManager* pm     = nullptr;
    Game* g               = nullptr;

    try {
        {
            auto devs = std::move(pi.devices);
            std::unique_ptr<Device> defaultdev;

            if (devs.size() == 0) {
                log->write("main", LogType::Fatal, "no video devices found!");
                return 1;
            }

            for (auto& d : devs) {
                log->write(
                    "main", LogType::Info, "driver found: %s %s", d->getCode().data(),
                    (d->isDefault() ? "(default)" : ""));

                if (d->isDefault()) {
                    defaultdev = std::move(d);
                }
            }

            if (!defaultdev) {
                log->write(
                    "main", LogType::Warning,
                    "no default device available, choosing the first one");
                defaultdev = std::move(devs[0]);
            }
            GFXService::setDevice(std::move(defaultdev));
        }

        auto& device = GFXService::getDevice();

        auto ipr = std::make_unique<InputProcessor>();
        InputService::setInputManager(std::make_unique<InputManager>(*ipr.get()));
        auto& ima = InputService::getInputManager();

        //        InputManager::GetInstance()->Initialize();
        win = device->createWindow(pi.width, pi.height);

        win->show();
        // enable_gl_debug();

        if (!init_network()) {
            throw net_exception("Could not initialize network");
        }

        log->write("", LogType::Info, "Device name: %s", device->getName().data());
        log->write("", LogType::Info, "Device vendor: %s ", device->getVendor().data());

        int fwidth, fheight;
        int gwidth, gheight;
        win->getFramebufferSize(fwidth, fheight);
        win->getFramebufferSize(gwidth, gheight);

        Framebuffer* f3D  = GFXService::getDevice()->createFramebuffer("f3D", fwidth, fheight);
        Framebuffer* fGUI = GFXService::getDevice()->createFramebuffer("fGUI", gwidth, gheight);
        win->setFramebuffers(f3D, fGUI);

        guir = win->createGUIManager();
        guir->initialize(*win);
        // guir->initShaders(win);

        /* If we have a networked game ready, don't even show the main menu. */
        log->write(
            "texture", LogType::Info, "maximum tex size: %zu x %zu", Texture::GetMaximumSize(),
            Texture::GetMaximumSize());

        if (pi.mapFile || pi.inputFile) {
            int frames = 0;

            std::string mapFile = pi.inputFile ? "" : *pi.mapFile;

            auto createNormalSession_fn = [](logic::Terrain& map, auto& local_player_info) {
                /// running a normal game
                return initSinglePlayerSession(map, local_player_info);
            };

            Game* g = start_game(
                GraphicalInfo{f3D, fGUI, win, guir, (size_t)gwidth, (size_t)gheight}, lr,
                StartGameInfo{mapFile, pi.inputFile}, confdata, createNormalSession_fn);

            if (g) {
                lr.load([&]() { return g->runLoop(); });
                run_game_loop(lr, frames);
            }

            if (g) delete g;

            delete win;
            delete f3D;
            delete fGUI;
            fmt::print("\nExited. ({:d} frames)\n", frames);

        } else if (pi.serverAddress) {
            std::string addr = *pi.serverAddress;

            if (addr.find_first_of(':') == std::string::npos) {
                addr += ":8100";
            }

            fmt::print("Connecting to {}\n", addr);
            start_networked_game_cmdline(
                GraphicalInfo{f3D, fGUI, win, guir, (size_t)gwidth, (size_t)gheight}, addr,
                confdata);
        } else {
            return show_starting_menu(
                pi, GraphicalInfo{f3D, fGUI, win, guir, (size_t)gwidth, (size_t)gheight}, lr,
                confdata);
        }

    } catch (shader_exception& se) {
        log->write("init", LogType::Fatal, "Shader error: %s [d]", se.what());

        if (win) {
            std::string content = fmt::format(
                "Shader {} could not be compiled\n"
                "\n"
                "Error: {}",
                se.file, se.what());
            win->showMessageBox("Familyline Error", SysMessageBoxFlags::Error, content);
        }

        exit(EXIT_FAILURE);
    } catch (graphical_exception& we) {
        log->write("init", LogType::Fatal, "Window creation error: %s (d)", we.what());

        if (win) {
            std::string content = fmt::format("Graphical error: {}\n", we.what());
            win->showMessageBox("Familyline Error", SysMessageBoxFlags::Error, content);
        }

        exit(EXIT_FAILURE);
    } catch (logic_exception& le) {
        log->write("init", LogType::Fatal, "Logic exception: %s (d)", le.what());

        if (win) {
            std::string content = fmt::format(
                "An error happened in the simulation part of the game:\n"
                "\n"
                "Error: {}",
                le.what());
            win->showMessageBox("Familyline Error", SysMessageBoxFlags::Error, content);
        }

        exit(EXIT_FAILURE);
    } catch (net_exception& le) {
        log->write("init", LogType::Fatal, "Network exception: %s (d)", le.what());

        if (win) {
            std::string content = fmt::format(
                "A network-related fatal error happened:\n"
                "\n"
                "Error: {}",
                le.what());
            win->showMessageBox("Familyline Error", SysMessageBoxFlags::Error, content);
        }

        exit(EXIT_FAILURE);
    } catch (std::bad_alloc& be) {
        log->write("init", LogType::Fatal, "Allocation error: %s", be.what());

        log->write("init", LogType::Fatal, "Probably out of memory");

        if (win) {
            std::string content = fmt::format("Insufficient memory\n\nError: {:s}", be.what());
            win->showMessageBox("Familyline Error", SysMessageBoxFlags::Error, content);
        }

        exit(EXIT_FAILURE);
    }

    end_network();
}

static int show_starting_menu(
    const ParamInfo& pi, GraphicalInfo ginfo, LoopRunner& lr, ConfigData& confdata)
{
    std::thread async_test;
    std::atomic<int> val    = 0;
    std::atomic<bool> aexit = false;

    auto sf = ServerFinder();

    auto& log = LoggerService::getLogger();
    Game* g   = nullptr;
    auto& ima = InputService::getInputManager();
    /* Render the menu */
    bool r = true;
    // auto deflistener = InputManager::GetInstance()->GetDefaultListener();

    GUIWindow* gwin = ginfo.guir->createGUIWindow("main", ginfo.gwidth, ginfo.gheight);
    CServer cserv{};

    Label* l = new Label(0.37, 0.03, "FAMILYLINE");
    l->modifyAppearance([](ControlAppearance& ca) {
        ca.fontSize   = 32;
        ca.foreground = {1, 1, 1, 1};
    });

    Label* lv = new Label(0.32, 0.8, "Version " VERSION ", commit " COMMIT);
    lv->modifyAppearance([](ControlAppearance& ca) {
        ca.foreground = {0.2, 0.2, 1, 1};
        ca.background = {1, 1, 1, 0.5};
    });

    Button* bnew      = new Button(400, 50, "New Game");  // Button(0.1, 0.2, 0.8, 0.1, "New Game");
    Button* bsettings = new Button(400, 50, "Settings");  // Button(0.1, 0.2, 0.8, 0.1, "New Game");
    Button* bmplayer =
        new Button(400, 50, "Multiplayer");            // Button(0.1, 0.2, 0.8, 0.1, "New Game");
    Button* bquit = new Button(400, 50, "Exit Game");  // Button(0.1, 0.31, 0.8, 0.1, "Exit Game");

    ImageView* ilogo = new ImageView(300, 450);  // 0.2, 0.1, 0.6, 0.9,
    ilogo->loadFromFile(ICONS_DIR "/familyline-logo.png");

    ilogo->z_index = -100;
    // ilogo.SetZIndex(0.9);
    // ilogo.SetOpacity(0.5);

    bquit->setClickCallback([&r](Control* cc) {
        (void)cc;
        r = false;
    });

    bsettings->setClickCallback([&](auto* cc) {
        GUIWindow* gsettings = ginfo.guir->createGUIWindow("settings", ginfo.gwidth, ginfo.gheight);
        // TODO: copy label?
        auto lb = std::make_unique<Label>(0.37, 0.03, "FAMILYLINE");
        lb->modifyAppearance([](ControlAppearance& ca) {
            ca.fontSize   = 32;
            ca.foreground = {1, 1, 1, 1};
        });

        auto header = std::make_unique<Label>(0.37, 0.03, "Settings");
        header->modifyAppearance([](ControlAppearance& ca) {
            ca.fontSize   = 24;
            ca.foreground = {1, 1, 1, 0.9};
        });

        auto txtname = std::make_unique<Textbox>(600, 40, confdata.player.username);

        auto bret =
            std::make_unique<Button>(200, 50, "Return");  // Button(0.1, 0.2, 0.8, 0.1, "New Game");

        bret->setClickCallback([&](auto* c) {
            GUIWindow* gsettings          = ginfo.guir->getGUIWindow("settings");
            Checkbox* recordGame          = (Checkbox*)gsettings->get("recordGame");
            Textbox* txtuname             = (Textbox*)gsettings->get("txtuname");
            confdata.enableInputRecording = recordGame->getState();
            confdata.player.username      = txtuname->getText();
            ginfo.guir->closeWindow(*gsettings);
            ginfo.guir->destroyGUIWindow("settings");
        });

        gsettings->add(0.37, 0.03, ControlPositioning::CenterX, std::move(lb));
        gsettings->add(0.37, 0.13, ControlPositioning::CenterX, std::move(header));
        gsettings->add(
            0.05, 0.3, ControlPositioning::Relative,
            std::make_unique<Checkbox>(
                300, 32, "Record the game inputs", confdata.enableInputRecording),
            "recordGame");
        gsettings->add(0.05, 0.4, ControlPositioning::Relative, std::move(txtname), "txtuname");
        gsettings->add(0.37, 0.9, ControlPositioning::CenterX, std::move(bret));

        ginfo.guir->showWindow(gsettings);
    });

    std::map<std::string, ServerInfo> silist;
    std::mutex simtx;
    bmplayer->setClickCallback([&](auto* cc) {
        simtx.lock();
        silist.clear();
        simtx.unlock();

        GUIWindow* gmplayer = ginfo.guir->createGUIWindow("mplayer", ginfo.gwidth, ginfo.gheight);
        // TODO: copy label?
        auto lb = std::make_unique<Label>(0.37, 0.03, "FAMILYLINE");
        lb->modifyAppearance([](ControlAppearance& ca) {
            ca.fontSize   = 32;
            ca.foreground = {1, 1, 1, 1};
        });

        auto header = std::make_unique<Label>(0.37, 0.03, "Multiplayer");
        header->modifyAppearance([](ControlAppearance& ca) {
            ca.fontSize   = 24;
            ca.foreground = {1, 1, 1, 0.9};
        });

        auto info = std::make_unique<Label>(
            0.37, 0.03, "Select a server from the list below, and click in 'Connect'");
        info->modifyAppearance([](ControlAppearance& ca) { ca.foreground = {1, 1, 1, 1}; });

        auto disclbl = std::make_unique<Label>(
            0.37, 0.03, "<i>Discovering servers in your local network...</i>");
        disclbl->modifyAppearance([](ControlAppearance& ca) { ca.foreground = {1, 1, 1, 1}; });

        auto txtaddr             = std::make_unique<Textbox>(300, 40, "");
        auto serverlist          = std::make_unique<Listbox>(800 * 0.7, 600 * 0.4);
        serverlist->onSelectItem = [&](std::string addr) {
            GUIWindow* gmplayer = ginfo.guir->getGUIWindow("mplayer");

            simtx.lock();
            ServerInfo si        = silist[addr];
            std::string addrport = fmt::format("{}:{}", si.ip_addr, si.port);
            simtx.unlock();

            if (gmplayer) ((Textbox*)gmplayer->get("txtaddr"))->setText(addrport);
        };

        aexit = false;
        val   = 0;

        auto bconnect = std::make_unique<Button>(
            200, 50, "Connect");  // Button(0.1, 0.2, 0.8, 0.1, "New Game");
        bconnect->modifyAppearance([](ControlAppearance& ca) {
            ca.foreground = {0, 0, 0, 0.9};
            ca.background = {1, 1, 1, 0.9};
        });
        bconnect->setClickCallback([&](auto* c) {
            Button* b = (Button*)c;

            GUIWindow* gmplayer = ginfo.guir->getGUIWindow("mplayer");
            auto addr           = ((Textbox*)gmplayer->get("txtaddr"))->getText();
            if (addr.size() < 3) return;

            b->setText("Connecting...");

            auto errHandler = [&](std::string msg, NetResult ret) {
                switch (ret) {
                    case NetResult::ConnectionError:
                        ginfo.win->showMessageBox(
                            msg, SysMessageBoxFlags::Warning,
                            fmt::format("Could not connect to address {}: Connection error", addr));
                        break;
                    case NetResult::WrongPassword:
                        ginfo.win->showMessageBox(
                            msg, SysMessageBoxFlags::Warning,
                            fmt::format("Server {} has a password", addr));
                        break;
                    case NetResult::LoginFailure:
                        ginfo.win->showMessageBox(
                            msg, SysMessageBoxFlags::Warning,
                            fmt::format(
                                "Could not log in to {}\nThe server was found, but we could not "
                                "login there.",
                                addr));
                        break;
                    case NetResult::ConnectionTimeout:
                        ginfo.win->showMessageBox(
                            msg, SysMessageBoxFlags::Warning,
                            fmt::format(
                                "Timed out while connecting to {}\nThe server probably does not "
                                "exist, or there is a firewall issue.",
                                addr));
                        break;
                    case NetResult::ServerError:
                        ginfo.win->showMessageBox(
                            msg, SysMessageBoxFlags::Warning,
                            fmt::format(
                                "Error while connecting to {}. The server sent incorrect data to "
                                "the client.",
                                addr));
                        break;
                    case NetResult::NotAllClientsConnected:
                        ginfo.win->showMessageBox(
                            msg, SysMessageBoxFlags::Warning,
                            fmt::format(
                                "Error while connecting to {}. The server reported that not all "
                                "clients were connected, but the client and the server disagree on "
                                "that.",
                                addr));
                        break;
                    default:
                        ginfo.win->showMessageBox(
                            msg, SysMessageBoxFlags::Warning,
                            fmt::format(
                                "Could not connect to address {}: unknown error {}", addr, ret));
                        break;
                }
            };

            auto ret = cserv.login(addr, confdata.player.username);
            if (ret != NetResult::OK) {
                ((Textbox*)gmplayer->get("txtaddr"))->setText("");
                b->setText("Connect");
                errHandler("Connection error", ret);
                return;
            }

            start_networked_game_room(ginfo, cserv, errHandler, confdata);

            cserv.logout();
            b->setText("Connect...");
        });

        auto bret =
            std::make_unique<Button>(200, 30, "Return");  // Button(0.1, 0.2, 0.8, 0.1, "New Game");

        bret->setClickCallback([&](auto* c) {
            GUIWindow* gmplayer = ginfo.guir->getGUIWindow("mplayer");
            Listbox* slist      = (Listbox*)gmplayer->get("serverlist");

            aexit = true;
            printf("%s ==== \n", slist->getSelectedItem().c_str());

            sf.stopDiscover();
            ginfo.guir->closeWindow(*gmplayer);
            ginfo.guir->destroyGUIWindow("mplayer");
        });

        gmplayer->add(0.37, 0.03, ControlPositioning::CenterX, std::move(lb));
        gmplayer->add(0.37, 0.13, ControlPositioning::CenterX, std::move(header));
        gmplayer->add(0.05, 0.2, ControlPositioning::Relative, std::move(info));
        gmplayer->add(
            0.05, 0.25, ControlPositioning::Relative, std::move(serverlist), "serverlist");
        gmplayer->add(0.6, 0.75, ControlPositioning::Relative, std::move(bconnect));
        gmplayer->add(0.05, 0.75, ControlPositioning::Relative, std::move(txtaddr), "txtaddr");
        gmplayer->add(0.05, 0.85, ControlPositioning::Relative, std::move(disclbl));
        gmplayer->add(0.37, 0.9, ControlPositioning::CenterX, std::move(bret));

        sf.startDiscover([&](ServerInfo si) {
            fmt::print(
                " \033[1m{}\033[0m ({}/{})\n{}:{}\n\n", si.name, si.player_count, si.player_max,
                si.ip_addr, si.port);

            GUIWindow* gmplayer = ginfo.guir->getGUIWindow("mplayer");
            Listbox* slist      = (Listbox*)gmplayer->get("serverlist");

            simtx.lock();
            silist[si.ip_addr] = si;
            simtx.unlock();

            slist->addItem(
                si.ip_addr, std::make_unique<Label>(
                                0, 0,
                                fmt::format(
                                    "<b>{}</b>     - {}:{} <span foreground='cyan'>({}/{})</span>",
                                    si.name, si.ip_addr, si.port, si.player_count, si.player_max)));
        });

        ginfo.guir->showWindow(gmplayer);
    });

    bnew->setClickCallback([&](Control* cc) {
        (void)cc;
        if (async_test.joinable()) {
            if (!aexit) {
                aexit = true;
                async_test.join();
            }
        }

        ginfo.guir->closeWindow(*gwin);

        auto createNormalSession_fn = [](logic::Terrain& map, auto& local_player_info) {
            /// running a normal game
            return initSinglePlayerSession(map, local_player_info);
        };

        g = start_game(
            ginfo, lr, StartGameInfo{ASSET_FILE_DIR "terrain_test.flte", std::nullopt}, confdata,
            createNormalSession_fn);

        if (g) {
            lr.load([&]() { return g->runLoop(); });
        }
    });

    gwin->add(0.37, 0.03, ControlPositioning::CenterX, std::unique_ptr<Control>((Control*)l));
    gwin->add(0.32, 0.8, ControlPositioning::Relative, std::unique_ptr<Control>((Control*)lv));
    gwin->add(0.1, 0.2, ControlPositioning::CenterX, std::unique_ptr<Control>((Control*)bnew));
    gwin->add(
        0.1, 0.305, ControlPositioning::CenterX, std::unique_ptr<Control>((Control*)bsettings));
    gwin->add(0.1, 0.41, ControlPositioning::CenterX, std::unique_ptr<Control>((Control*)bmplayer));
    gwin->add(0.1, 0.52, ControlPositioning::CenterX, std::unique_ptr<Control>((Control*)bquit));
    gwin->add(0.2, 0.01, ControlPositioning::CenterX, std::unique_ptr<Control>((Control*)ilogo));

    ginfo.guir->showWindow(gwin);
    // guir->add(0, 0, ControlPositioning::Pixel, std::unique_ptr<Control>((Control*)gwin));

    ima->addListenerHandler([&](HumanInputAction hia) {
        /* Only listen for game exit events, because you sure want to
           close the window The others will be handled by the GUI listener */
        if (std::holds_alternative<GameExit>(hia.type)) {
            r = false;
        }

        return false;
    });

    double b   = SDL_GetTicks();
    int frames = 0;

    lr.load([&]() {
        // Input
        ima->processEvents();

        ginfo.guir->receiveEvent();
        ginfo.guir->runCallbacks();
        ginfo.guir->update();

        // Render
        ginfo.fGUI->startDraw();
        ginfo.guir->render(0, 0);
        // guir->renderToScreen();
        ginfo.fGUI->endDraw();

        ginfo.win->update();

        double e = SDL_GetTicks();
        if ((e - b) < 1000 / 60.0) SDL_Delay((unsigned int)(1000 / 60.0 - (e - b)));
        b = SDL_GetTicks();
        return r;
    });

    run_game_loop(lr, frames);

    if (async_test.joinable()) {
        if (!aexit) {
            aexit = true;
            async_test.join();
        }
    }

    if (g) delete g;

    delete ginfo.win;
    delete ginfo.f3D;
    delete ginfo.fGUI;
    fmt::print("\nExited. ({:d} frames)\n", frames);

    return 0;
}
