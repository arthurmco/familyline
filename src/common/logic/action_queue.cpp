#include <fmt/format.h>

#include <algorithm>
#include <common/logger.hpp>
#include <common/logic/action_queue.hpp>
#include <functional>

using namespace familyline::logic;

void ActionQueue::addEmitter(EventEmitter* e)
{
    // printf("\tadded event emitter %s\n", e->getName().c_str());

    e->queue = this;
}

void ActionQueue::addReceiver(EventReceiver* r, std::initializer_list<ActionQueueEvent> events)
{
    ReceiverData rd;
    rd.events   = events;
    rd.receiver = r;

    // printf("\tadded event receiver %s\n", r->getName().c_str());

    this->receivers.push_back(rd);
}

void ActionQueue::removeReceiver(EventReceiver* r)
{
    auto newend = std::remove_if(receivers.begin(), receivers.end(), [r](ReceiverData rec) {
        return (rec.receiver->getName() == r->getName());
    });
    receivers.erase(newend, receivers.end());
}

void ActionQueue::removeEmitter(EventEmitter* e) { e->queue = nullptr; }

/// This will allow us to use std::visit with multiple variants at once, a thing
/// that should be part of C++20.
template <class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

void ActionQueue::pushEvent(const EntityEvent& ev)
{
    std::string begin = fmt::format(
        "timestamp={}, source={}", ev.timestamp,
        ev.emitter ? ev.emitter->getName().c_str() : "(null emitter)");

    auto& log = LoggerService::getLogger();
    std::visit(
        overload{
            [&](const EventCreated& e) {
                log->write(
                    "action-queue", LogType::Info, "event added: EventCreated (%s, objectID=%llu)",
                    begin.c_str(), e.objectID);
            },
            [&](const EventBuilding& e) {
                log->write(
                    "action-queue", LogType::Info, "event added: EventBuilding (%s, objectID=%llu)",
                    begin.c_str(), e.objectID);
            },
            [&](const EventBuilt& e) {
                log->write(
                    "action-queue", LogType::Info, "event added: EventBuilt (%s, objectID=%llu)",
                    begin.c_str(), e.objectID);
            },
            [&](const EventReady& e) {
                log->write(
                    "action-queue", LogType::Info, "event added: EventReady (%s, objectID=%llu)",
                    begin.c_str(), e.objectID);
            },
            [&](const EventAttacking& e) {
                log->write(
                    "action-queue", LogType::Info,
                    "event added: EventAttacking (%s, "
                    "attacker(id=%llu, xpos=%d, ypos=%d), defender(id=%llu, xpos=%d, ypos=%d),"
                    "damageDealt=%.2f)",
                    begin.c_str(), e.attackerID, e.atkXPos, e.atkYPos, e.defenderID, e.defXPos,
                    e.defYPos, e.damageDealt);
            },
            [&](const EventWorking& e) {
                log->write(
                    "action-queue", LogType::Info,
                    "event added: EventWorking (%s, objectID=%llu,"
                    "atkXPos=%d, atkYPos=%d",
                    begin.c_str(), e.objectID, e.atkXPos, e.atkYPos);
            },
            [&](const EventGarrisoned& e) {
                log->write(
                    "action-queue", LogType::Info,
                    "event added: EventGarrisoned (%s, objectID=%llu,"
                    "parentID=%d, entering=%s",
                    begin.c_str(), e.objectID, e.parentID, e.entering ? "true" : "false");
            },
            [&](const EventDying& e) {
                log->write(
                    "action-queue", LogType::Info,
                    "event added: EventDying (%s, objectID=%llu,"
                    "atkXPos=%d, atkYPos=%d",
                    begin.c_str(), e.objectID, e.atkXPos, e.atkYPos);
            },
            [&](const EventDead& e) {
                log->write(
                    "action-queue", LogType::Info, "event added: EventDead (%s, objectID=%llu)",
                    begin.c_str(), e.objectID);
            },
            [&](const EventDestroyed& e) {
                log->write(
                    "action-queue", LogType::Info,
                    "event added: EventDestroyed (%s, objectID=%llu)", begin.c_str(), e.objectID);
            }},
        ev.type);

    this->events.push(ev);
}

void ActionQueue::processEvents()
{
    while (!this->events.empty()) {
        EntityEvent e = this->events.front();

        // printf("event %x !\n", e.type);

        for (auto& rec : this->receivers) {
            if (std::find(rec.events.begin(), rec.events.end(), e.type.index()) !=
                rec.events.end()) {
                // printf("\t received by %s\n", rec.receiver->getName().c_str());
                rec.receiver->pushEvent(e);
            }
        }

        this->events.pop();
    }
}
