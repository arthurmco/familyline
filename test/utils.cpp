#include "utils.hpp"

using namespace familyline::logic;

TestObject::TestObject(const struct object_init& init)
    : GameObject(init.type, init.name, init.size, init.health, init.maxHealth, init.showHealth),
      init_params_(init)
{
    this->fnUpdate_ = init.fnUpdate;

    if (init.atkComponent) {
        this->cAttack                = init.atkComponent;
        this->cAttack.value().object = this;
    }
}

void TestObject::doUpdate() { fnUpdate_(); }

std::shared_ptr<GameObject> TestObject::create()
{
    auto r = std::make_shared<TestObject>(init_params_);
    r->_id = -1;

    return r;
}

std::shared_ptr<TestObject> make_object(const struct object_init& init)
{
    return std::make_shared<TestObject>(init);
}

TestOwnableObject::TestOwnableObject(const struct object_init& init)
    : GameObject(init.type, init.name, init.size, init.health, init.maxHealth, init.showHealth),
      init_params_(init)
{
    this->fnUpdate_ = init.fnUpdate;

    if (init.atkComponent) {
        this->cAttack                = init.atkComponent;
        this->cAttack.value().object = this;
    }

    this->cColony = std::make_optional(ColonyComponent());
}

void TestOwnableObject::doUpdate() { fnUpdate_(); }

std::shared_ptr<GameObject> TestOwnableObject::create()
{
    auto r = std::make_shared<TestOwnableObject>(init_params_);
    r->_id = -1;

    return r;
}

std::shared_ptr<TestOwnableObject> make_ownable_object(const struct object_init& init)
{
    return std::make_shared<TestOwnableObject>(init);
}

void DummyPlayer::generateInput()
{
    /// Make sure that the input callback is called once per tick
    if (last_tick == this->getTick())
        return;

    last_tick = this->getTick();
    
    auto input = player_input_cb_(last_tick);

    for (auto& i : input) {
        this->pushAction(i);
    }
}

