#include "Geometry.h"
#include "GraphicsContext.h"

namespace acqua
{
	Geometry::Geometry(void) :
		graphicsContext(NULL)
		, indexBuffer(0)
		, vertexBuffer(0)
		, vertexArray(0)
		, vertexLayout(0)
		, indexCount(0)
		, vertexCount(0)
	{
	}


	Geometry::~Geometry(void)
	{
	}

	bool Geometry::Load(GraphicsContext* graphics_context, const void* vertex_data, u32 num_vertices, const void* index_data, u32 num_indices, const VertexLayoutAttrib* attributes, u32 num_attributes)
	{
		if(graphics_context == NULL)
		{
			ASSERT(0, "Graphics Context shouldn't be null");
			return false;
		}

		graphicsContext = graphics_context;

		vertexLayout = graphicsContext->AddVertexLayout(num_attributes, attributes);
		ASSERT(vertexLayout != 0, "The Vertex Layout has an invalid handle!");

		u32 vertex_size = graphicsContext->GetVertexLayout(vertexLayout).size;

		vertexBuffer = graphicsContext->CreateVertexBuffer(vertex_size * num_vertices, vertex_data); 
		ASSERT(vertexBuffer != 0, "The Vertex Buffer has an invalid handle");

		indexBuffer  = graphicsContext->CreateIndexBuffer(sizeof(u32) * num_indices, index_data);
		ASSERT(indexBuffer != 0, "The Index Buffer has an invalid handle");

		vertexArray = graphicsContext->CreateVertexArray(vertexBuffer, indexBuffer, vertexLayout);
		ASSERT(vertexArray != 0, "The Vertex Array has an invalid handle");

		vertexCount = num_vertices;
		indexCount = num_indices;

		return true;
	}

	void Geometry::UpdateVertexData( void* vertex_data, u32 offset )
	{
		ASSERT(graphicsContext != NULL, "Graphics Context cannot be null.");
		
		u32 vertex_size = graphicsContext->GetVertexLayout(vertexLayout).size;

		graphicsContext->UpdateBufferData(vertexBuffer, offset, vertex_size * vertexCount, vertex_data);
	}

}