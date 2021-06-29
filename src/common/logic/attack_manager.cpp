#include <algorithm>
#include <common/logger.hpp>
#include <common/logic/attack_manager.hpp>
#include <common/logic/game_object.hpp>
#include <common/logic/logic_service.hpp>
#include "common/logic/game_event.hpp"

using namespace familyline::logic;

AttackManagerEventReceiver::AttackManagerEventReceiver()
{
    LogicService::getActionQueue()->addReceiver((EventReceiver*)this, {ActionQueueEvent::Destroyed});
}

const std::string AttackManagerEventReceiver::getName() { return _name; }

AttackManager::AttackManager()
{
    ame_receiver = new AttackManagerEventReceiver{};
    ame_emitter  = new AttackManagerEventEmitter{};
}

AttackManager::~AttackManager()
{
    LogicService::getActionQueue()->removeReceiver(ame_receiver);
    LogicService::getActionQueue()->removeEmitter(ame_emitter);
    delete ame_receiver;
    delete ame_emitter;
}

std::optional<double> AttackManager::attack(AttackComponent& defender)
{
    return std::optional<double>();
}

void AttackManager::doRegister(int oid, AttackComponent& atk) { this->components[oid] = &atk; }

attack_handle_t AttackManager::startAttack(int attackerOID, int defenderOID)
{
    auto atkhandle = make_attack_handle(attackerOID, defenderOID);

    LoggerService::getLogger()->write(
        "attack-manager", LogType::Info, "attack started: object ID %d will attack object ID %d",
        attackerOID, defenderOID);

    AttackData adata;
    adata.atk = this->components[attackerOID];
    adata.def = this->components[defenderOID];

    assert(adata.atk != nullptr);
    assert(adata.def != nullptr);

    this->attacks[atkhandle] = adata;

    return atkhandle;
}

void AttackManager::endAttack(attack_handle_t ahandle) { this->attacks.erase(ahandle); }

/**
 * Check if any object has been deleted and should be removed
 */
void AttackManager::checkRemovedObjects()
{
    EntityEvent e;

    std::vector<std::tuple<attack_handle_t, int>> to_remove;
    while (this->ame_receiver->pollEvent(e)) {
        // The event types are guaranteed to be only the object destruction events
        auto* atk = std::get_if<EventAttacking>(&e.type);
        if (!atk)
            continue;
        
        
        auto eid = atk->attackerID;

        for (auto [ahandle, adata] : this->attacks) {
            int atkID, defID;
            std::tie(atkID, defID) = break_attack_handle(ahandle);

            if (eid == atkID || eid == defID) {
                to_remove.push_back(std::make_tuple(ahandle, eid));
            }
        }

        for (auto [cid, _unused] : this->components) {
            bool is_removing = std::find_if(to_remove.begin(), to_remove.end(), [eid](auto a) {
                                   return std::get<1>(a) == eid;
                               }) == to_remove.end();

            if (cid == eid && is_removing) {
                to_remove.push_back(std::make_tuple(0, eid));
            }
        }
    }

    for (auto [handle, oid] : to_remove) {
        LoggerService::getLogger()->write(
            "attack-manager", LogType::Debug, "removed %x %lx\n", oid, handle);

        if (handle > 0) this->attacks.erase(handle);

        this->components.erase(oid);
    }
}

void AttackManager::processAttacks(ObjectLifecycleManager& olm)
{
    this->checkRemovedObjects();
    auto& log = LoggerService::getLogger();

    std::vector<attack_handle_t> to_remove;
    for (auto [ahandle, adata] : this->attacks) {
        auto dmg = adata.atk->doDirectAttack(*adata.def);
        if (!dmg) {
            log->write("attack-manager", LogType::Debug, "out of range");
            to_remove.push_back(ahandle);
            continue;
        }

        auto vdmg = dmg.value_or(0);
        if (vdmg == 0.0) {
            log->write("attack-manager", LogType::Debug, "damage is null");
            to_remove.push_back(ahandle);
            continue;
        }

        adata.def->object->addHealth(-vdmg);
        this->ame_emitter->generateAttackEvent(adata.atk, adata.def, vdmg);

        // When dying, set the health to zero and remove the object
        if (adata.def->object->getHealth() <= 0) {
            GameObject* go = adata.def->object;

            go->addHealth(-adata.def->object->getHealth());
            log->write("attack-manager", LogType::Debug, "the defender is dead");
            components.erase(go->getID());
            to_remove.push_back(ahandle);

            olm.notifyDeath(go->getID());

            continue;
        }
    }

    for (auto handle : to_remove) {
        log->write("attack-manager", LogType::Debug, "removed %lx\n", handle);
        this->attacks.erase(handle);
    }
}

attack_handle_t familyline::logic::make_attack_handle(int attackerOID, int defenderOID)
{
    static_assert(sizeof(int) * 2 == sizeof(attack_handle_t), "Mismatched size for attack handle");

    return defenderOID | (((attack_handle_t)attackerOID) << 32);
}

/**
 * Split an attack handle to the attacker and defender
 */
std::tuple<int, int> familyline::logic::break_attack_handle(attack_handle_t handle)
{
    return std::make_tuple<int, int>(handle >> 32, handle & 0xffffffff);
}

AttackManagerEventEmitter::AttackManagerEventEmitter()
{
    LogicService::getActionQueue()->addEmitter((EventEmitter*)this);
}

const std::string AttackManagerEventEmitter::getName() { return _name; }

void AttackManagerEventEmitter::generateAttackEvent(
    AttackComponent* atk, AttackComponent* def, double dmg)
{
    auto atkpos = atk->object->getPosition();
    auto defpos = def->object->getPosition();


    EntityEvent eatk{0, EventAttacking{
            atk->object->getID(),
            def->object->getID(),
            unsigned(atkpos.x), unsigned(atkpos.z),
            unsigned(defpos.x), unsigned(defpos.z),
            dmg
        }, nullptr};
    this->pushEvent(eatk);


    if (def->object->getHealth() <= 0.0) {
        EntityEvent edying{
            0, EventDying{
                def->object->getID(),
                unsigned(defpos.x), unsigned(defpos.z)
            },
            nullptr
        };

        this->pushEvent(edying);
    }

}
