#include "TerrainComponent.h"
#include "GraphicsContext.h"
#include "GameObject.h"

#include <SFML\Graphics.hpp>

namespace acqua
{
	TerrainComponent::TerrainComponent(void) : Component(CT_TERRAINCOMPONENT)
		, vertices(NULL)
		, vertexCount(0)
		, indices(NULL)
		, indexCount(0)
		, sizeInUnits(3.0f)
		, heightScale(3.0f * 500.0f)
	{
	}


	TerrainComponent::~TerrainComponent(void)
	{
		delete [] vertices;
		delete [] indices;
	}

	bool TerrainComponent::Init( GameObject* o )
	{
		bool result = Component::Init(o);
		return result;
	}

	void TerrainComponent::FixedUpdate( float fixed_delta_time )
	{
	
	}

	void TerrainComponent::Update( float delta_time )
	{

	}

	void TerrainComponent::Draw( float delta_time )
	{

	}

	void TerrainComponent::LoadFromHeightMap( std::string filename )
	{
		sf::Image heightmap;
		if(!heightmap.loadFromFile(filename))
			return;

		u32 map_width  = heightmap.getSize().x;
		u32 map_height = heightmap.getSize().y;

		float terrain_width = sizeInUnits * (map_width - 1);
		float terrain_height = sizeInUnits * (map_height - 1);

		float half_terrain_width = terrain_width * 0.5f;
		float half_terrain_height = terrain_height * 0.5f;

		vertices = new VertexTerrain[map_width * map_height];

		for(u32 j = 0; j < map_height; ++j)
		{
			for(u32 i = 0; i < map_width; ++i)
			{
				u32 index = i + (j * map_width);
				// Generate geometry information.
				// Texture coordinates.
				float u = i / static_cast<float>(map_width - 1);
				float v = j / static_cast<float>(map_height - 1);

				float height = 0.0f;
				for(int u_j = j - 0; u_j <= j + 0; ++u_j)
				{
					for(int u_i = i - 0; u_i <= i + 0; ++u_i)
					{
						if(u_j < 0 || u_j >= map_height || u_i < 0 || u_i >= map_width)
							continue;

						height += heightmap.getPixel(u_i, u_j).r / static_cast<float>(255);
						height *= 0.5f;
					}
				}
				//height = height >= 0.0001f ? (height > 1.0f ? 1.0f : height) : 0.0001f;
				float x = (u * terrain_width) - half_terrain_width;
				float y = height * heightScale;
				float z = (v * terrain_height) - half_terrain_height;

				vertices[index].position = glm::vec3(x, y, z);
				vertices[index].normal = glm::vec3(0.0f, 1.0f, 0.0f);
				vertices[index].texcoords = glm::vec2(u, v);

				++vertexCount;
			}
		}

		// Generate indices.
		const u32 num_triangles = (map_width - 1) * (map_height - 1) * 2;
		indices = new u32[num_triangles * 3];
		
		u32 index = 0;
		for(u32 j = 0; j < map_height - 1; ++j)
		{
			for(u32 i = 0; i < map_width - 1; ++i)
			{
				u32 vertex_index =  i + (j * map_width);
				indices[index++] = vertex_index;
				indices[index++] = vertex_index + map_width;
				indices[index++] = vertex_index + 1;

				indices[index++] = vertex_index + map_width;
				indices[index++] = vertex_index + map_width + 1;
				indices[index++] = vertex_index + 1;
			}
		}
		indexCount = index - 1;

		// Generate normals.
		for ( unsigned int i = 0; i < indexCount; i += 3 )
		{
			glm::vec3 v0 = vertices[ indices[i + 0] ].position;
			glm::vec3 v1 = vertices[ indices[i + 1] ].position;
			glm::vec3 v2 = vertices[ indices[i + 2] ].position;

			glm::vec3 normal = glm::normalize( glm::cross( v1 - v0, v2 - v0 ) );

			vertices[ indices[i + 0] ].normal += normal;
			vertices[ indices[i + 1] ].normal += normal;
			vertices[ indices[i + 2] ].normal += normal;
		}

		const glm::vec3 UP( 0.0f, 1.0f, 0.0f );
		for ( unsigned int i = 0; i < vertexCount; ++i )
		{
			vertices[i].normal = glm::normalize( vertices[i].normal );
		}

		// Vertex attribute.
		VertexLayoutAttrib vertex_attributes[] = 
		{
			{3, 0},
			{3, sizeof(glm::vec3)},
			{2, 2 * sizeof(glm::vec3)}
		};
		u32 num_vertex_attributes = sizeof(vertex_attributes) / sizeof(VertexLayoutAttrib);

		// We need geometry.
		geometry = std::make_shared<Geometry>();
		geometry->Load(owner->GetScene().GetGraphicsContext(), vertices, vertexCount, indices, indexCount, vertex_attributes, num_vertex_attributes);

		owner->AddComponent("GeometryRenderer");
		GeometryRenderer* geometry_render = owner->GetComponent<GeometryRenderer>();
		if(geometry_render == NULL)
		{
			delete [] vertices;
			vertexCount = 0;
			delete [] indices;
			indexCount = 0;
			geometry = NULL;
			return;
		}

		geometry_render->SetGeometry(geometry);
	}

}
