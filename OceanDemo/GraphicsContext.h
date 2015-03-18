#pragma once

#include "Shader.h"

#include "Types.h"
#include "DebugUtil.h"

#include <glm\glm.hpp>

#include <vector>

namespace acqua
{
	// Forward declarations.
	class Geometry;

	// Holds Graphics Context's objects keeping a freelist.
	template<typename T>
	class GCObjects
	{
	public:
		// Add an object and returns its handle.
		u32 Add( const T& object )
		{
			if(!freeList.empty())
			{
				u32 handle = freeList.back();
				freeList.pop_back();
				objects[handle] = object;
				return handle + 1;
			}
			else
			{
				objects.push_back(object);
				return static_cast<u32>(objects.size());
			}
		}

		void Remove(u32 handle)
		{
			ASSERT(handle > 0 && handle <= objects.size(), "Hanlde is invalid");

			handle = handle - 1;
			objects[handle] = T(); //Calls destructor. Replaces with default constructed object.
			freeList.push_back(handle);
		}

		T& GetRef(u32 handle)
		{
			ASSERT(handle > 0 && handle <= objects.size(), "Hanlde is invalid");

			return objects[handle - 1];
		}

		u32 Size()
		{
			return static_cast<u32>(objects.size());
		}

	private:
		std::vector<T>		objects;
		std::vector<u32>	freeList;
	};

	// OpenGL buffer object
	struct GCBuffer
	{
		u32 type;
		u32 glObj; // OpenGL Buffer Object
		u32 size;
	};

	// OpenGL Vertex Array
	struct GCVertexArray
	{
		u32 glObj;
		u32 vertexBuffer;	// Handle to the vertex buffer.
		u32 indexBuffer;	// Handle to the index buffer.
		u32 vertexLayout;	// Handle to the vertex layout.
	};

	// Vertex Layout
	struct VertexLayoutAttrib
	{
		u32 size;
		u32 offset;
	};

	struct GCVertexLayout
	{
		u32					numAttribs;
		u32					size;			// Used for the stride
		VertexLayoutAttrib	attribs[16];
	};

	// Textures

	struct TextureTypes
	{
		enum List
		{
			Tex2D	=	GL_TEXTURE_2D,
			Tex3D	=	GL_TEXTURE_3D,
			TexCube	=	GL_TEXTURE_CUBE_MAP,
		};
	};

	struct TextureFormats
	{
		enum List
		{
			Unknown,
			RGB,
			RGBA,
			BGRA8,
			RGBA16F,
			RGBA32F,
			DEPTH
		};
	};

	struct GCTexture
	{
		u32						glObj;
		u32						glFormat;
		u32		                type;
		TextureFormats::List	format;
		u32						width, height, depth;
		u32						memSize;
		u32		                samplerState;
		bool					sRGB;
		bool					hasMips, genMips;
	};

	// Render Buffers.
	struct GCRenderBuffer
	{
		static const u32 kMaxColorAttachments = 4;

		u32 fbo, fboMS;		// OpenGL Frame Buffer objects. MS is for multisampling.
		u32 width, height;	// Dimensions.
		u32 samples;		// When > 0 uses the fboMS.

		u32 depthTexture;	// Depth Texture.
		u32 depthBuffer;	// Depth Renderbuffer.

		u32 colorTextures[kMaxColorAttachments];	// Color attachments (textures).
		u32 colorBuffers[kMaxColorAttachments];		// Color attachments (renderbuffers).

		GCRenderBuffer() : fbo(0), fboMS(0), width(0), height(0), samples(0), depthBuffer(0), depthTexture(0)
		{
			for (u32 i = 0; i < kMaxColorAttachments; ++i)
			{
				colorTextures[i] = colorBuffers[i] = 0;
			}
		}
	};

	// This represents an OpenGL 4 Graphics Context
	class GraphicsContext
	{
	public:
		GraphicsContext(void);
		~GraphicsContext(void);

		bool Init();

		void Clear(); // Clear all the buffers (if writing on them is enabled).

		// Buffers.
		u32 CreateVertexBuffer(u32 size, const void* data);
		u32 CreateIndexBuffer(u32 size, const void* data);

		void DestroyBuffer(u32 handle);

		void UpdateBufferData(u32 handle, u32 offset, u32 size, void *data);

		// Vertex Arrays.
		u32 CreateVertexArray(u32 vertex_buffer, u32 index_buffer, u32 vertex_layout);

		// Vertex Layouts.
		u32 AddVertexLayout(u32 num_attribs, const VertexLayoutAttrib* attribs);
		GCVertexLayout& GetVertexLayout(u32 vertex_layout_handle) { return vertexLayouts.GetRef(vertex_layout_handle); }

		// Shaders.
		u32 CreateShader(ShaderType type, const char* source);
		Shader& GetShader(u32 handle) { return shaders.GetRef(handle); }

		u32 CreateShaderProgram(u32* shader_handles, u32 num_shaders);
		ShaderProgram& GetShaderProgram(u32 handle) { return shaderPrograms.GetRef(handle); }

		void UseShaderProgram(u32 shader_program_handle);

		u32 GetCurrentShaderProgram() const { return currentProgram; }

		// Textures.
		u32 CreateTexture(TextureTypes::List type, int width, int height, int depth, TextureFormats::List format, bool has_mips, bool gen_mips, bool compress, bool sRGB);
		void UploadTextureData(u32 handle, int slice, int mip_level, const void *pixels);

		void UseTexture(u32 slot, u32 handle); //TODO: Make this proper.

		void DestroyTexture( u32 handle );
		
		GCTexture& GetTexture( u32 handle ) { return textures.GetRef(handle); }

		// Render buffers.
		u32 CreateRenderbuffer(u32 width, u32 height, TextureFormats::List format, bool depth, u32 num_color_buffers, u32 samples = 0);
		void DestroyRenderBuffer(u32 handle);
		u32 GetRenderbufferTexture(u32 handle, u32 tex_index);
		void SetRenderBuffer(u32 handle);


		// API State.
		const glm::vec4& ClearColour() const { return clearColour; }
		void SetClearColour(const glm::vec4& clear_colour);

		u8 GetColourMask() const { return colourMask; }
		void SetColourMask(bool red, bool green, bool blue, bool alpha);

		float ClearDepth() const { return clearDepth; }
		void SetClearDepth(float clear_depth );

		bool GetDepthMask() const { return depthMask; }
		void SetDepthMask(bool depth_mask);

		// Drawing.
		void ToggleWireframeDrawing();
		bool IsWireframeEnabled() const { return wireframe; }

		void DrawGeometry(const Geometry* geometry, u32 num_patches = 0);

		// Accessors.
		void SetScreenSize(u32 w, u32 h) { screenWidth = w; screenHeight = h; }
		u32 GetScreenWidth() const { return screenWidth; }
		u32 GetScreenHeight() const { return screenHeight; }

	private:
		// Buffers.
		u32 CreateBuffer(GCBuffer& buffer, u32 size, const void* data);
		
	private:
		// Device variables.
		u32 screenWidth;
		u32 screenHeight;

		// State variables. 
		// TODO: Consider structuring this.
		glm::vec4	clearColour;
		u8			colourMask; // Is colour write enabled? (0000ABGR layout)

		float	clearDepth;
		bool	depthMask;

		bool wireframe;

		// Objects and Buffers.
		GCObjects<GCBuffer>			buffers;		// Holds the OpenGL buffers.
		GCObjects<GCVertexArray>	vertexArrays;	// Holds the OpenGL Vertex Array Objects
		GCObjects<GCVertexLayout>	vertexLayouts;	// Holds the various vertex layouts.

		// Shaders.
		GCObjects<Shader>			shaders; // Holds GLSL shaders.
		GCObjects<ShaderProgram>	shaderPrograms; // Holds GLSL Shader Programs.

		// Textures.
		GCObjects<GCTexture>		textures;

		// Render buffers.
		GCObjects<GCRenderBuffer>	renderBuffers;
		u32							defaultRenderbuffer; // Represents the screen.
		u32							currentRenderbuffer; 

		u32 currentProgram;
	};
}