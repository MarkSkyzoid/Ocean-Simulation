#pragma once

#include "Types.h"

#include <list>

namespace acqua
{
	// Forward declarations.
	class GraphicsContext;
	class GameObject;
	class GeometryRenderer;
	class CameraComponent;

	// Represents a scene in the game.
	class Scene
	{
	public:
		Scene(void);
		~Scene(void);

		bool Init(/* TODO: Engine* owner */ GraphicsContext* graphics_context);

		// Creates a GameObject in the scene and allows the user to add components to it by returning a reference.
		GameObject& CreateGameObject(); 
		
		// Messages.
		void RegisterGeometryRenderer(GeometryRenderer* geometry_renderer);
		void UnregisterGeometryRenderer(GeometryRenderer* geometry_renderer);

		void RegisterCamera(CameraComponent* camera);
		void UnregisterCamera(CameraComponent* camera);
		
		void ResolutionChanged(int width, int height);

		// Updating and Drawing.
		void FixedUpdate(float fixed_delta_time);
		void Update(float delta_time);

		void Draw(float delta_time);

		// Accessors.
		GraphicsContext* GetGraphicsContext() { return graphicsContext; }

	private:
		// TODO: Functions and data that should be in a High Level Renderer class. Aww Marco...
		void SetGeneralUniforms(const CameraComponent* camera, float time); // Set things such as viewProjectionMatrix, camera position etc.

		// Attributes;

	private:
		std::list<GameObject*> gameObjects; // GameObjects in the scene.
		std::list<const GeometryRenderer*> geometryRenderers; // Keeps a list for drawing.

		typedef std::list<CameraComponent*> CameraList;
		typedef CameraList::iterator CameraListIterator;
		CameraList cameras; // Camera Components. Can have different viewports.
		
		GraphicsContext* graphicsContext;

		float renderTime; // Accumulator of time for the render process.

		// Renderbuffers for water optics.
		u32 reflectionBuffer; //Used for local reflections.
		u32 refractionBuffer; //Encodes refraction colour and linear depth (alpha channel).

		u32 foamBuffer; //Half-res buffer with foam intensity.
	};
}