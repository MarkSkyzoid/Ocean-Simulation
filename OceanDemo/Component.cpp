#include "Component.h"

#include <map>
#include <string>

namespace acqua
{
	typedef std::map<std::string, PComponentFactory> ComponentFactoryMap;

	struct ComponentFactorySingleton
	{
		static ComponentFactoryMap& get()
		{
			static ComponentFactoryMap cfmap;
			return cfmap;
		}
	} componentFactorySingleton;

	PComponent Component::Create(const char* name)
	{
		std::string n(name);
		ComponentFactoryMap::iterator it = componentFactorySingleton.get().find(n);
		if(it == componentFactorySingleton.get().end())
			return NULL;

		return it->second->MakeComponent();
	}

	// Component class implementation.

	bool Component::Init( GameObject* o )
	{
		ASSERT(o != NULL, "A Component cannot be created outside a GameObject.");

		owner = o; 
		return true;
	}


	void Component::Register(const char* name, PComponentFactory factory)
	{
		componentFactorySingleton.get()[std::string(name)] = factory;
	}
}