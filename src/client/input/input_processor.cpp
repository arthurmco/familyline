#include <client/input/input_processor.hpp>
#include <chrono>
#include <SDL2/SDL.h>

using namespace familyline::input;

void InputProcessor::startInputReceiver()
{
    _runThread = std::thread(
        [&]() {
            _isRunning = true;

            int lastX, lastY;

            SDL_Event e;
            while (_isRunning) {
                if (!SDL_PollEvent(&e)) {
                    SDL_Delay(10);
                    continue;
                }

                auto duration =  std::chrono::system_clock::now().time_since_epoch();
                uint64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

                switch (e.type) {
                case SDL_MOUSEMOTION:
                    lastX = e.motion.x;
                    lastY = e.motion.y;
                    _actions.push({millis, MouseAction{e.motion.x, e.motion.y}});
                    break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    bool isPressed = (e.button.state == SDL_PRESSED);
                    _actions.push({millis, ClickAction{e.motion.x, e.motion.y, e.button.button,
                                                       e.button.clicks, isPressed}});
                }
                    break;
                case SDL_MOUSEWHEEL: {
                    bool dirNormal = e.wheel.direction == SDL_MOUSEWHEEL_NORMAL;
                    _actions.push({millis, WheelAction{
                                lastX, lastY,
                                (dirNormal) ? e.wheel.x : -e.wheel.x,
                                (dirNormal) ? e.wheel.y : -e.wheel.y,
                            }});
                }
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP: {
                    bool isPressed = (e.key.state == SDL_PRESSED);
                    bool isRepeated = (e.key.repeat > 0);
                    uint16_t modifiers = e.key.keysym.mod;
                    auto keycode = e.key.keysym.sym;                    
                    _actions.push({millis, KeyAction{
                            keycode, SDL_GetKeyName(keycode),
                            isPressed, isRepeated, modifiers
                            }});

                }
                    break;
                    
                case SDL_QUIT:
                    fprintf(stderr, "event: GameExit\n");
                    _actions.push({millis, GameExit{0}});
                    _isRunning = false;
                    break;
                    
                case SDL_WINDOWEVENT:

                    const char* winevent;

                    switch (e.window.event) {
                    case SDL_WINDOWEVENT_SHOWN:
                        winevent = "SDL_WINDOWEVENT_SHOWN";
                        break;                        
                    case SDL_WINDOWEVENT_HIDDEN:
                        winevent = "SDL_WINDOWEVENT_HIDDEN";
                        break;
                    case SDL_WINDOWEVENT_EXPOSED:
                        winevent = "SDL_WINDOWEVENT_EXPOSED";
                        break;
                    case SDL_WINDOWEVENT_MOVED:                        
                        winevent = "SDL_WINDOWEVENT_MOVED (x, y)";
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        winevent = "SDL_WINDOWEVENT_RESIZED (x, y)";
                        break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        winevent = "SDL_WINDOWEVENT_SIZE_CHANGED (w, h)";
                        break;
                    case SDL_WINDOWEVENT_MINIMIZED:
                        winevent = "SDL_WINDOWEVENT_MINIMIZED";
                        break;
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        winevent = "SDL_WINDOWEVENT_MAXIMIZED";
                        break;
                    case SDL_WINDOWEVENT_RESTORED:
                        winevent = "SDL_WINDOWEVENT_RESTORED";
                        break;
                    case SDL_WINDOWEVENT_ENTER:
                        winevent = "SDL_WINDOWEVENT_ENTER (windowID)";
                        break;
                    case SDL_WINDOWEVENT_LEAVE:
                        winevent = "SDL_WINDOWEVENT_LEAVE (windowID)";
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        winevent = "SDL_WINDOWEVENT_FOCUS_GAINED (windowID)";
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        winevent = "SDL_WINDOWEVENT_FOCUS_LOST (windowID)";
                        break;
                    case SDL_WINDOWEVENT_CLOSE:
                        winevent = "SDL_WINDOWEVENT_CLOSE (windowID)";
                        break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
                    case SDL_WINDOWEVENT_TAKE_FOCUS:
                        winevent = "SDL_WINDOWEVENT_TAKE_FOCUS (windowID)";
                        break;
                    case SDL_WINDOWEVENT_HIT_TEST:
                        winevent = "SDL_WINDOWEVENT_HIT_TEST (windowID)";
                        break;
#endif
                    default:
                        winevent = "unknown";
                        break;
                    }
                    
                    
                    fprintf(stderr, "SDL_WindowEvent: %s (id %08x), event %08x, data %08x,%08x\n",
                            winevent, e.window.windowID, e.window.event,
                            e.window.data1, e.window.data2);

                    break;
                default:
                    fprintf(stderr, "unknown event id %08x\n", e.type);
                }

            }

        });
}

void InputProcessor::stopInputReceiver()
{
    _isRunning = false;
    _runThread.join();
}


bool InputProcessor::pollAction(HumanInputAction& a)
{
    if (_actions.empty())
        return false;

    a = _actions.front();
    _actions.pop();
    return true;
}