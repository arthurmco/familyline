/*	Represents an object factory
 
	Copyright (C) 2016 Arthur M
*/

#include <map>

#include "GameObject.hpp"

#ifndef OBJECTFACTORY_HPP
#define OBJECTFACTORY_HPP

namespace Tribalia {
namespace Logic {
	
class ObjectFactory {
private:
	std::map<int /*tid*/, GameObject*> _objects;

public:
	/* Gets an instance of object with type 'typeID', or
	 * nullptr if given object wasn't added to the factory
	 * i.e, doesn't exist */
	GameObject* GetObject(int typeID);

	/* Adds an object to the factory */
	void AddObject(GameObject* object);

	static ObjectFactory* GetInstance() {
		static ObjectFactory* f = nullptr;
		if (!f) 	f = new ObjectFactory;

		return f;
	}
};

}
}

#endif
 
