#pragma once

#include "Types.h"

namespace acqua
{
	// Forward declarations.
	struct	VertexLayoutAttrib;
	class	GraphicsContext;

	// Represents geometry.
	class Geometry
	{
	public:
		Geometry(void);
		~Geometry(void);

		// TODO: One day, make a file format and stream a bunch of data, please.
		bool Load(GraphicsContext* graphics_context, const void* vertex_data, u32 num_vertices, const void* index_data, u32 num_indices, const VertexLayoutAttrib* attributes, u32 num_attributes);

		void UpdateVertexData(void* vertex_data, u32 offset);

	private:
		GraphicsContext* graphicsContext; // Owner.

		u32 indexBuffer;	// Handle to the index buffer resource in the Graphics Context.
		u32 vertexBuffer;	// Handle to the vertex buffer resource in the Graphics Context.
		u32 vertexArray;	// Handle to the vertex array resource in the Graphics Context.

		u32 vertexLayout;	// Handle to the vertex layout resource in the Graphics Context.

		u32 indexCount;		// Number of indices.
		u32 vertexCount;	// Number of vertices.

	private:
		friend class GraphicsContext;
	}; 
}
