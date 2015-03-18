#pragma once

#include "Types.h"
#include "Scene.h"
#include "Component.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <map>

namespace acqua
{
	// Forward Declarations.
	class Geometry;
	class GraphicsContext;
	class CameraComponent;

	// A transform represents the position, orientation and scale of each game object.
	class Transform
	{
	public:
		enum TransformDirtyFlags
		{
			TDC_NONE		= 0,
			TDC_POSITION	= 1 << 0, // 1
			TDC_SCALE		= 1 << 1, // 2
			TDC_ORIENTATION	= 1 << 2, // 4
			TDC_ALL			= TDC_POSITION | TDC_SCALE | TDC_ORIENTATION,
		};

	public:
		Transform(void);
		~Transform(void);

		void MarkDirty();
		void MarkDirty(TransformDirtyFlags dirty_flag);

		bool IsDirty() const { return (dirtyFlags != TDC_NONE); }

		// Accessors.
		const glm::mat4& GetMatrix() const;
		
		void SetPosition(glm::vec3 pos);
		void SetOrientation(glm::quat or);
		void SetEulerAngles(float x, float y, float z);
		void SetAxisRotation(const glm::vec3& axis, float angle);
		
		const glm::vec3& GetPosition() const { return position; }
		const glm::quat& GetOrientation() const { return orientation; }
		glm::vec3 GetEulerAngles() const;

		const glm::vec3& GetForward() const { return forward; }
		const glm::vec3& GetUp() const { return up; }
		const glm::vec3& GetRight() const { return right; }

	private:
		void ResetDirtyFlag() const { dirtyFlags = TDC_NONE; }
		void RecomputeMatrix() const;
	
	private:
		mutable u16 dirtyFlags;

		mutable glm::mat4 transformMatrix; // The final transform matrix.

		glm::quat orientation;
		glm::vec3 position;
		glm::vec3 scale;

		glm::vec3 forward;
		glm::vec3 up;
		glm::vec3 right;
	};
	
	// Reference to geometry used and properties.
	class GeometryRenderer : public Component
	{
	public:
		GeometryRenderer(void) : Component(CT_GEOMTRYRENDERER), shader_program(0) {}
		~GeometryRenderer(void);

		// Base class virtual methods.
		bool Init(GameObject* o);

		virtual void FixedUpdate(float fixed_delta_time) {}
		virtual void Update(float delta_time) {}

		virtual void Draw(float delta_time);

		// Component methods.
		void SetGeometry(std::shared_ptr<Geometry> geom) { geometry = geom; }
		const std::shared_ptr<Geometry> GetGeometry() const {return geometry;}

		void SetShaderProgram(u32 handle) { shader_program = handle; }
		u32 GetShaderProgram() const { return shader_program; }

	private:
		std::shared_ptr<Geometry> geometry;

		u32 shader_program;
	
	private:
		friend class Scene;
	};

	REGISTER_COMPONENT(GeometryRenderer)

	class GameObject
	{
	public:
		GameObject(void);
		~GameObject(void);

		Transform& GetTransform() { return transform; }
		const Transform& GetTransform() const { return transform; }

		bool AddComponent(const char* name);

		template <typename T>
		T* GetComponent()
		{
			T c;
			T* component = NULL;

			std::map<ComponentType, Component*>::iterator it;
			it = components.find(c.GetType());
			if(it != components.end())
			{
				component = dynamic_cast<T*>(it->second);
			}

			return component;
		}

		template <typename T>
		bool HasComponent() const 
		{
			T c;
			std::map<ComponentType, Component*>::const_iterator it;
			it = components.find(c.GetType());
			if(it != components.end())
			{
				return true;
			}

			return false;
		}

		Scene& GetScene(); // Assumes scene is not NULL as a game object cannot live outside a scene.

		void FixedUpdate(float fixed_delta_time);
		void Update(float delta_time);

		void Draw(float delta_time);

	private:
		Transform transform;
		std::map<ComponentType, Component*> components;

		Scene* owner; // The owning scene.

	private:
		friend class Scene;
	};
}


