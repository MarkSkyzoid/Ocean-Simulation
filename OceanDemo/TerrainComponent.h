#pragma once

#include "Component.h"
#include "Types.h"
#include "Geometry.h"

#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace acqua
{
	class TerrainComponent : public Component
	{
	private:
		struct VertexTerrain
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 texcoords;
		};

	public:
		TerrainComponent(void);
		~TerrainComponent(void);

		virtual bool Init( GameObject* o );

		virtual void FixedUpdate( float fixed_delta_time );

		virtual void Update( float delta_time );

		virtual void Draw( float delta_time );

		// Load terrain.
		void LoadFromHeightMap(std::string filename);

	private:
		// Geometry generation.
		VertexTerrain*	vertices;
		u32				vertexCount;
		u32*			indices;
		u32				indexCount;

		std::shared_ptr<Geometry> geometry;

		// Terrain parameters.
		float sizeInUnits;
		float heightScale;
	}; 

	REGISTER_COMPONENT(TerrainComponent)
}


