#include "InputManager.hpp"

using namespace Tribalia::Input;

/* Get the top event (not taking it off the queue).
    Return false if no elements on queue */
bool InputManager::GetEvent(InputEvent* ev)
{
    if (_evt_queue.empty())
        return false;

    *ev = _evt_queue.front();

    // printf("\t event: ");
    // switch (ev->eventType) {
    //     case EVENT_MOUSEEVENT:
    //         printf("mouse, button %d , status %d ",
    //             ev->event.mouseev.button, ev->event.mouseev.status);
    //         break;
    //
    //     case EVENT_KEYEVENT:
    //         printf("keyboard, scancode %d (char %c), status %d",
    //             ev->event.keyev.scancode,
    //             (char)ev->event.keyev.char_utf8 & 0xff,
    //             ev->event.keyev.status);
    //         break;
    //
    //     case EVENT_FINISH:
    //         printf("finish requested");
    //         break;
    //
    //     default:
    //         printf("unknown");
    //         break;
    // }
    //
    // printf(", mouse coords (%d %d %d)\n",
    //     ev->mousex, ev->mousey, ev->mousez);

    return true;
}

/* Pop off the top element of the queue
    Return false if there's no element to pop off */
bool InputManager::PopEvent(InputEvent* ev)
{

    if (_evt_queue.empty())
        return false;

    if (ev)
        *ev = _evt_queue.front();

    _evt_queue.pop();
    return true;
}

int lastx, lasty, lastz;

/* Receive events and send them to queues */
void InputManager::Run()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        InputEvent ev;
        switch (e.type) {
        case SDL_QUIT:
        case SDL_APP_TERMINATING:
            ev.eventType = EVENT_FINISH;
            break;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_CLOSE)
                ev.eventType = EVENT_FINISH;
            else
                continue;
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            ev.eventType = EVENT_KEYEVENT;

            ev.event.keyev.scancode = e.key.keysym.sym;
            ev.event.keyev.status = (e.key.state == SDL_PRESSED) ?
                KEY_KEYPRESS : KEY_KEYRELEASE;
            if (e.key.repeat > 0)
                ev.event.keyev.status = KEY_KEYREPEAT;

            ev.event.keyev.char_utf8 = ' ';
            break;
        case SDL_MOUSEMOTION:
            /* Only update the last* variables */
            lastx = e.motion.x;
            lasty = e.motion.y;
            lastz = 0;
            continue;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            ev.eventType = EVENT_MOUSEEVENT;
            //i will not depend of hacks.
            switch (e.button.button) {
                case SDL_BUTTON_LEFT:
                    ev.event.mouseev.button = MOUSE_LEFT;
                    break;
                case SDL_BUTTON_MIDDLE:
                    ev.event.mouseev.button = MOUSE_MIDDLE;
                    break;
                case SDL_BUTTON_RIGHT:
                    ev.event.mouseev.button = MOUSE_RIGHT;
                    break;
                default:
                    ev.event.mouseev.button = e.button.button - MOUSE_LEFT;
            }

            lastx = e.button.x;
            lasty = e.button.y;

            ev.event.mouseev.status = (e.button.state == SDL_PRESSED) ?
                KEY_KEYPRESS : KEY_KEYRELEASE;

            if (e.button.clicks % 2 == 0)
                ev.event.keyev.status = KEY_KEYREPEAT;
        default:
            continue;
    }

        ev.mousex = lastx;
        ev.mousey = lasty;
        ev.mousez = lastz;

        _evt_queue.push(ev);


    }


}