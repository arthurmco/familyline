#include <algorithm>
#include <common/logic/object_factory.hpp>

using namespace familyline::logic;

/* Gets an instance of object with type 'type', or
 * nullptr if given object wasn't added to the factory
 * i.e, doesn't exist */
std::shared_ptr<GameObject> ObjectFactory::getObject(const char* type, float x, float y, float z)
{
    if (_objects.find(type) != _objects.end()) {
        auto newo = _objects[type]->create();
        newo->setPosition(glm::vec3(x, y, z));
        return newo;
    }

    return nullptr;
}

/* Adds an object to the factory */
void ObjectFactory::addObject(GameObject* object) { _objects[object->getType()] = object; }

/**
 * Get a key of types and object checksums
 */
std::map<std::string, object_checksum_t> ObjectFactory::getObjectChecksums() const
{
    std::map<std::string, object_checksum_t> ret;

    std::for_each(_objects.begin(), _objects.end(), [&]
                   (const std::pair<std::string, GameObject*>& val) {
        ret[val.first] = val.second->getChecksum();
    });

    return ret;
}
