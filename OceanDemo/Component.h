#pragma once

#include "DebugUtil.h"

namespace acqua
{
	// GameObject Component base class.
	class GameObject;

	enum ComponentType
	{
		CT_NONE,
		CT_GEOMTRYRENDERER,
		CT_CAMERA,
		CT_OCEANCOMPONENT,
		CT_TERRAINCOMPONENT,
		CT_COUNT
	};

	class Component;
	typedef Component* PComponent;

	class ComponentFactory
	{
	public:
		virtual PComponent MakeComponent() = 0;
	};

	typedef ComponentFactory* PComponentFactory;

	template<typename T>
	class GenericComponentFactory : public ComponentFactory
	{
	public:
		virtual PComponent MakeComponent()
		{
			PComponent component = new T();
			return component;
		}
	};


	class Component
	{
	public:
		Component(ComponentType type) : componentType(type) {}
		virtual ~Component(void) {}

		virtual bool Init(GameObject* o);

		virtual void FixedUpdate(float fixed_delta_time) = 0;
		virtual void Update(float delta_time) = 0;

		virtual void Draw(float delta_time) = 0;

		// Accessors.
		ComponentType GetType() const { return componentType; }
		const GameObject& GetGameObject() const { return *owner; }
		GameObject& GetGameObject() { return *owner; }

		// Factory Methods
		static PComponent Create(const char* name);
		static void	Register(const char* name, PComponentFactory factory);

	protected:
		GameObject* owner;
		ComponentType componentType;
	};

	template <typename T>
	class RegisterComponentFactory
	{
	public:
		RegisterComponentFactory(const char* name)
		{
			PComponentFactory factory = new GenericComponentFactory<T>();
			Component::Register(name, factory);
		}
	};
#define REGISTER_COMPONENT(type) \
	namespace  \
	{ \
	static RegisterComponentFactory<type> fac_##type(#type); \
	}
}
