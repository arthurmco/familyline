#include <chrono>
#include <common/logic/action_queue.hpp>
#include <common/logic/game_event.hpp>

using namespace familyline::logic;

void EventEmitter::pushEvent(EntityEvent& e)
{
    auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();

    e.timestamp = epoch;
    e.emitter   = this;

    this->queue->pushEvent(e);
}

bool EventReceiver::pollEvent(EntityEvent& e)
{
    if (events.empty()) return false;

    EntityEvent tmp = events.front();
    events.pop();
    e = tmp;
    return true;
}

void EventReceiver::pushEvent(EntityEvent& e) { events.push(e); }
