#pragma once

#include "Component.h"

#include "Types.h"

#include <glm\glm.hpp>
#include <memory>

namespace acqua
{
	// Forward declarations.
	class GraphicsContext;
	class Geometry;

	// Special Component for cameras.
	class CameraComponent : public Component
	{
	public:
		struct CameraViewport
		{
			// All of this in pixels.
			int x;
			int y;
			int width;
			int height;
		};

	public:
		CameraComponent(void);
		~CameraComponent(void);

		virtual bool Init(GameObject* o);

		virtual void FixedUpdate( float fixed_delta_time );

		virtual void Update( float delta_time );

		virtual void Draw( float delta_time );
	
		void MarkDirty() { dirtyFlag = true; }
		bool IsDirty() const { return dirtyFlag; }

		// Accessors.
		const glm::mat4& GetViewProjectionMatrix() const;
		const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return viewMatrix; }
		
		const CameraViewport& GetViewport() const { return viewport; }
		void SetViewport(const CameraViewport& other_viewport) { viewport = other_viewport; MarkDirty(); }

		float GetFieldOfView() const { return fieldOfView; }
		void  SetFieldOfView(float fov) { fieldOfView = fov; MarkDirty(); }

		float GetNearPlane() const { return nearPlane; }
		void SetNearPlane(float near_plane) { nearPlane = near_plane; MarkDirty(); }

		float GetFarPlane() const { return farPlane; }
		void SetFarPlane(float far_plane) { farPlane = far_plane; MarkDirty(); }

		// Skybox methods.
		void GenerateSkybox(u32 cubemap_handle);
		void DrawSkybox(GraphicsContext* graphics_context);

	private:
		void RecomputeViewMatrix();
		void RecomputeProjectionMatrix();

		static void LoadSkyboxShader(GraphicsContext* graphics_context);

	private:
		glm::mat4 viewProjectionMatrix;
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;
		CameraViewport viewport;
		float fieldOfView;
		bool dirtyFlag;
		float nearPlane;
		float farPlane;

		// Skybox fields.
		bool						hasSkybox;
		u32							skyboxCubemap; // Handle to the texture.
		std::shared_ptr<Geometry>	skyboxGeometry; // Skybox geometry.

		static bool					skyboxShaderLoaded;
		static u32					skyboxShader; // Handle to the skybox shader. It's static as it's the same for every instance.
	};

	REGISTER_COMPONENT(CameraComponent)
}