#include <algorithm>
#include <common/logic/game_object.hpp>

using namespace familyline::logic;

GameObject::GameObject(
    std::string type, std::string name, glm::vec2 size, int health, int maxHealth, bool showHealth)
    : _type(type),
      _name(name),
      _size(size),
      _health(health),
      _maxHealth(maxHealth),
      _showHealth(showHealth)
{
    this->cLocation                = std::make_optional<LocationComponent>();
    this->cLocation.value().object = this;

    this->cAttack                = std::make_optional<AttackComponent>();
    this->cAttack->range         = 3.14;
    this->cAttack.value().object = this;
}

/**
 * Function to be called by the ObjectFactory, so it can
 * create a game object without knowing its class name,
 * considering you registered it there
 *
 * \see ObjectFactory
 */
std::shared_ptr<GameObject> GameObject::create()
{
    auto cloned = std::make_shared<GameObject>(
        this->_name, this->_type, this->_size, this->_health, this->_maxHealth, this->_showHealth);

    // TODO: copy the components.
    cloned->cAttack = this->cAttack ? std::make_optional<AttackComponent>(this->cAttack.value())
                                    : std::optional<AttackComponent>();
    cloned->cContainer = this->cContainer
                             ? std::make_optional<ContainerComponent>(this->cContainer.value())
                             : std::optional<ContainerComponent>();
    cloned->cLocation = this->cLocation
                            ? std::make_optional<LocationComponent>(this->cLocation.value())
                            : std::optional<LocationComponent>();
    cloned->cMovement = this->cMovement
                            ? std::make_optional<MovementComponent>(this->cMovement.value())
                            : std::optional<MovementComponent>();

    if (cloned->cAttack) cloned->cAttack->object = this;

    if (cloned->cLocation) cloned->cLocation->object = this;

    return cloned;
}

/**
 * Get the "object checksum"
 *
 * This checksum will be used by network games and the input reproducer
 * to see if this object is the same as the objects used in the other
 * clients, or in the client who recorded the inputs.
 *
 * If the checksum matches, it will mean that the components
 * were equal, and then the simulation will succeed.
 */
object_checksum_t GameObject::getChecksum() const
{
    char vatk[192] = {};
    if (this->cAttack) {
        auto& atk = *this->cAttack;
        snprintf(
            vatk, 190, "%.3f|%.3f|%.3f|%.3f||%.3f|%.3f|%.3f|%.3f||%.3f|%.3f|%.3f|%.3f",
            atk.atkRanged, atk.atkMelee, atk.atkSiege, atk.atkTransporter, atk.defRanged,
            atk.defMelee, atk.defSiege, atk.defTransporter, atk.rotation, atk.atkDistance,
            atk.armor, atk.range);
    } else {
        snprintf(vatk, 190, "%180s", "false");
    }

    char vmov[16] = {};
    if (this->cMovement) {
        snprintf(vmov, 16, "%.4f", this->cMovement->speed);
    }

    object_checksum_t vchecksum = {};
    snprintf((char*)vchecksum.data(), 255, "%s;%s;%s", this->getType().c_str(), vatk, vmov);

    return vchecksum;
}

/**
 * Update internal object logic
 */
void GameObject::update() { this->doUpdate(); }
