#include "GraphicsContext.h"
#include "Geometry.h"

#include "GLUtil.h"

#include <GL\glew.h>
#include <algorithm>

namespace acqua
{
	GraphicsContext::GraphicsContext(void) :
		screenWidth(0)
		, screenHeight(0)
		, colourMask(0x0F) // All enabled
		, clearDepth(1.0f)
		, depthMask(true)
		, wireframe(false)
		, defaultRenderbuffer(0)
		, currentRenderbuffer(0)
		, currentProgram(0) // Invalid Handle
	{
	}


	GraphicsContext::~GraphicsContext(void)
	{
		// TODO: Implement a release function. Delete all buffers and OpenGL Objects.
		// TODO: Make this prettier.
		for(u32 i = 0; i < vertexArrays.Size(); ++i)
		{
			GCVertexArray& vertex_array = vertexArrays.GetRef(i + 1);
			if(vertex_array.glObj != 0)
			{
				GL_CHECK(glDeleteVertexArrays(1, &vertex_array.glObj));
			}
		}

		for(u32 i = 1; i < buffers.Size(); ++i)
		{
			GCBuffer& buffer = buffers.GetRef(i + 1);
			if(buffer.glObj != 0)
			{
				GL_CHECK(glDeleteBuffers(1, &buffer.glObj));
			}
		}

		for(u32 i = 0; i < shaders.Size(); ++i)
		{
			Shader& shader = shaders.GetRef(i + 1);
			if(shader.glObj != 0)
			{
				shader.Destroy();
			}
		}

		for(u32 i = 0; i < textures.Size(); ++i)
		{
			DestroyTexture(i + 1);
		}

		for(u32 i = 0; i < renderBuffers.Size(); ++i)
		{
			DestroyRenderBuffer(i + 1);
		}
	}

	bool GraphicsContext::Init()
	{
		bool result = false;

		// Initialize GLEW
		glewExperimental = GL_TRUE;
		GLenum glew_result = glewInit();

		result = (glew_result == GLEW_OK);

		// Set the initital state
		// TODO: More work.
		if(result)
		{
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			SetClearColour(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
			SetColourMask(true, true, true, true);
		}

		return result;
	}

	void GraphicsContext::Clear()
	{
		u32 prev_buffers[4] = { 0 };

		if(currentRenderbuffer != 0)
		{
			const GCRenderBuffer &rb = renderBuffers.GetRef(currentRenderbuffer);

			// Store state of glDrawBuffers
			for( u32 i = 0; i < 4; ++i )
				GL_CHECK(glGetIntegerv(GL_DRAW_BUFFER0 + i, (int *)&prev_buffers[i]));
			
			u32 buffers[4], cnt = 0;

			if( rb.colorTextures[0] != 0 ) buffers[cnt++] = GL_COLOR_ATTACHMENT0;
			if( rb.colorTextures[1] != 0 ) buffers[cnt++] = GL_COLOR_ATTACHMENT1;
			if( rb.colorTextures[2] != 0 ) buffers[cnt++] = GL_COLOR_ATTACHMENT2;
			if( rb.colorTextures[3] != 0 ) buffers[cnt++] = GL_COLOR_ATTACHMENT3;

			if( cnt != 0 )
				GL_CHECK(glDrawBuffers(cnt, buffers));
		}

		GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

		// Restore state of glDrawBuffers
		if( currentRenderbuffer != 0x0 )
			GL_CHECK(glDrawBuffers(4, prev_buffers));

	}

	// Buffers.
	u32 GraphicsContext::CreateBuffer(GCBuffer& buffer, u32 size, const void* data)
	{
		glGenBuffers(1, &buffer.glObj);
		glBindBuffer(buffer.type, buffer.glObj);
		glBufferData(buffer.type, size, data, GL_DYNAMIC_DRAW);
		glBindBuffer(buffer.type, 0);

		return buffers.Add(buffer); // Returns the handle.
	}

	u32 GraphicsContext::CreateVertexBuffer(u32 size, const void* data)
	{
		// NOTE: Vertex Buffers are interleaved.
		GCBuffer buffer;
		buffer.type = GL_ARRAY_BUFFER;
		buffer.size = size;

		u32 handle = CreateBuffer(buffer, size, data);
		return handle;
	}

	u32 GraphicsContext::CreateIndexBuffer(u32 size, const void* data)
	{
		GCBuffer buffer;
		buffer.type = GL_ELEMENT_ARRAY_BUFFER;
		buffer.size = size;

		u32 handle = CreateBuffer(buffer, size, data);
		return handle;
	}

	void GraphicsContext::DestroyBuffer(u32 handle)
	{
		if(handle == 0)
			return;

		GCBuffer& buffer = buffers.GetRef(handle);
		
		glDeleteBuffers(1, &buffer.glObj);
		
		buffers.Remove(handle);
	}

	void GraphicsContext::UpdateBufferData( u32 handle, u32 offset, u32 size, void *data )
	{
		const GCBuffer& buf = buffers.GetRef(handle);
		ASSERT( offset + size <= buf.size );

		glBindBuffer( buf.type, buf.glObj );

		if( offset == 0 &&  size == buf.size )
		{
			// Replacing the whole buffer can help the driver to avoid pipeline stalls.
			glBufferData( buf.type, size, data, GL_DYNAMIC_DRAW );
			return;
		}

		glBufferSubData( buf.type, offset, size, data );
	}

	// Vertex Arrays.
	u32 GraphicsContext::CreateVertexArray(u32 vertex_buffer, u32 index_buffer, u32 vertex_layout)
	{
		ASSERT(vertex_buffer > 0 && vertex_buffer <= buffers.Size(), "Invalid vertex buffer handle");
		ASSERT(index_buffer > 0 && index_buffer <= buffers.Size(), "Invalid index_buffer handl");
		ASSERT(vertex_layout > 0 && vertex_layout <= vertexLayouts.Size(), "Invalid vertex_layout handle");

		GCBuffer&		ib = buffers.GetRef(index_buffer);
		GCBuffer&		vb = buffers.GetRef(vertex_buffer);
		GCVertexLayout& vl = vertexLayouts.GetRef(vertex_layout);

		GCVertexArray vao;

		glGenVertexArrays(1, &vao.glObj);
		glBindVertexArray(vao.glObj);

		glBindBuffer(ib.type, ib.glObj);
		glBindBuffer(vb.type, vb.glObj);

		//Apply Vertex Layout
		const u32 num_attribs = vl.numAttribs;
		for(u32 i = 0; i < num_attribs; ++i)
		{
			glEnableVertexAttribArray(i);

			const VertexLayoutAttrib& attrib = vl.attribs[i]; 
			glVertexAttribPointer(	i, attrib.size, GL_FLOAT, // TODO: Consider supporting other types other than float.
									GL_FALSE, vl.size, 
									(char*)NULL + attrib.offset );		
		}

		glBindVertexArray(0);

		glBindBuffer(ib.type, 0);
		glBindBuffer(vb.type, 0);

		return vertexArrays.Add(vao); // Return the handle to the VAO.
	}

	// Vertex Layouts.
	u32 GraphicsContext::AddVertexLayout(u32 num_attribs, const VertexLayoutAttrib* attribs)
	{
		GCVertexLayout vl;
		vl.numAttribs = num_attribs;

		u32 size = 0;

		for(u32 i = 0; i < num_attribs; ++i)
		{
			const VertexLayoutAttrib& attrib = attribs[i];
			size += sizeof(float) * attrib.size; // TODO: Consider supporting other types other than float.
			vl.attribs[i] = attrib;
		}

		vl.size = size;

		return vertexLayouts.Add(vl); // TODO: This does an extra copy. Maybe it's better to use a simple array in this class with a maximum size.
	}

	//	Shaders.
	u32 GraphicsContext::CreateShader(ShaderType type, const char* source)
	{
		Shader shader;
		bool shader_created = shader.Create(type, source);
		
		if(!shader_created)
			return 0; // Invalid handle.
		
		return shaders.Add(shader); // Return the handle to the shader resource.
	}

	u32 GraphicsContext::CreateShaderProgram(u32* shader_handles, u32 num_shaders)
	{
		ShaderProgram program;
		program.graphicsContext = this; // Set the owner.

		bool program_created = program.Create(shader_handles, num_shaders);
		if(!program_created)
			return 0; // Invalid handle.

		return shaderPrograms.Add(program);
	}

	void GraphicsContext::UseShaderProgram(u32 shader_program_handle)
	{
		// MAYBE TODO: Consider using an empty shader program if the parameter is 0.
		// However, this should probably never happen, as a program should always be active before rendering. 
		ShaderProgram& shader_program = shaderPrograms.GetRef(shader_program_handle);

		currentProgram = shader_program_handle;

		GL_CHECK(glUseProgram(shader_program.glObj));
	}

	// Accessors
	void GraphicsContext::SetClearColour( const glm::vec4& clear_colour )
	{
		clearColour = clear_colour;
		glClearColor(clearColour.r, clearColour.g, clearColour.b, clear_colour.a);
	}

	void GraphicsContext::SetColourMask( bool red, bool green, bool blue, bool alpha )
	{
		colourMask = (red) | (green << 1) | (blue << 2) | (alpha << 3);
		glColorMask( red, green, blue, alpha );
	}

	void GraphicsContext::SetClearDepth( float clear_depth )
	{
		clearDepth = clear_depth;
		glClearDepth(clearDepth);
	}

	void GraphicsContext::SetDepthMask(bool depth_mask)
	{
		depthMask = depthMask;
		glDepthMask(depthMask);
	}

	// Drawing.

	void GraphicsContext::ToggleWireframeDrawing()
	{
		wireframe = !wireframe;

		glPolygonMode( GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL );
	}

	void GraphicsContext::DrawGeometry(const Geometry* geometry, u32 num_patches /*= 0*/)
	{
		if(geometry == NULL)
			return;

		u32 vao = vertexArrays.GetRef(geometry->vertexArray).glObj;
		GL_CHECK(glBindVertexArray(vao));
		
		if(num_patches > 0)
			GL_CHECK(glPatchParameteri(GL_PATCH_VERTICES, num_patches));
		// TODO: Check if it is indexed or not.
		//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		GL_CHECK(glDrawElements(num_patches > 0 ? GL_PATCHES : GL_TRIANGLES, geometry->indexCount, GL_UNSIGNED_INT, 0));
	}

	// Textures.
	glm::u32 GraphicsContext::CreateTexture( TextureTypes::List type, int width, int height, int depth, TextureFormats::List format, bool has_mips, bool gen_mips, bool compress, bool sRGB )
	{
		ASSERT(depth > 0, "Depth must be greater than zero");

		GCTexture tex;
		tex.type = type;
		tex.format = format;
		tex.width = width;
		tex.height = height;
		tex.depth = depth;
		tex.sRGB = sRGB;
		tex.genMips = gen_mips;
		tex.hasMips = has_mips;

		switch( format )
		{
		case TextureFormats::RGB:
			tex.glFormat = GL_RGB;
			break;
		case TextureFormats::RGBA:
			tex.glFormat = GL_RGBA;
			break;
		case TextureFormats::BGRA8:
			tex.glFormat = tex.sRGB ? GL_SRGB8_ALPHA8_EXT : GL_RGBA8;
			break;
		case TextureFormats::RGBA16F:
			tex.glFormat = GL_RGBA16F_ARB;
			break;
		case TextureFormats::RGBA32F:
			tex.glFormat = GL_RGBA32F_ARB;
			break;
		case TextureFormats::DEPTH:
			tex.glFormat = GL_DEPTH_COMPONENT24;
			break;
		default:
			ASSERT( 0 );
			break;
		};


		u32 target = tex.type;
		
		GL_CHECK(glGenTextures(1, &tex.glObj));
		GL_CHECK(glBindTexture(target, tex.glObj));

		// TODO: All this needs to be done based on the sampler state. IMPLEMENT SAMPLER STATES.
		GL_CHECK(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

		GL_CHECK(glTexParameteri( target, GL_TEXTURE_WRAP_S, tex.type == TextureTypes::TexCube ? GL_CLAMP_TO_EDGE : GL_REPEAT)); 
		GL_CHECK(glTexParameteri( target, GL_TEXTURE_WRAP_T, tex.type == TextureTypes::TexCube ? GL_CLAMP_TO_EDGE : GL_REPEAT));
		GL_CHECK(glTexParameteri( target, GL_TEXTURE_WRAP_R, tex.type == TextureTypes::TexCube ? GL_CLAMP_TO_EDGE : GL_REPEAT));

		GL_CHECK(glBindTexture( tex.type, 0 ));

		return textures.Add(tex);
	}

	void GraphicsContext::UploadTextureData(u32 handle, int slice, int mip_level, const void *pixels)
	{
		const GCTexture& tex = textures.GetRef(handle);

		TextureFormats::List format = tex.format;

		GL_CHECK(glBindTexture(tex.type, tex.glObj));

		int input_format = GL_BGRA, input_type = GL_UNSIGNED_BYTE;

		switch( format )
		{
		case TextureFormats::RGBA16F:
		case TextureFormats::RGBA32F:
			input_format = GL_RGBA;
			input_type = GL_FLOAT;
			break;
		case TextureFormats::RGB:
			input_format = GL_RGB;
			input_type = GL_UNSIGNED_BYTE;
			break;
		case TextureFormats::RGBA:
			input_format = GL_RGBA;
			input_type = GL_UNSIGNED_BYTE;
			break;
		case TextureFormats::DEPTH:
			input_format = GL_DEPTH_COMPONENT;
			input_type = GL_FLOAT;
			break;
		};

		// Calculate size of next mipmap using "floor" 
		int width = (std::max<int>)( tex.width >> mip_level, 1 ), height = (std::max<int>)( tex.height >> mip_level, 1 );
		if( tex.type == TextureTypes::Tex2D || tex.type == TextureTypes::TexCube )
		{
			int target = (tex.type == TextureTypes::Tex2D) ? GL_TEXTURE_2D : (GL_TEXTURE_CUBE_MAP_POSITIVE_X + slice);
			GL_CHECK(glTexImage2D( target, mip_level, tex.glFormat, width, height, 0, input_format, input_type, pixels ));
		}
		// TODO: Implement 3D textures

		if( tex.genMips && (tex.type != GL_TEXTURE_CUBE_MAP || slice == 5) )
		{
			// Note: for cube maps mips are only generated when the side with the highest index is uploaded
			GL_CHECK((glEnable( tex.type )));  // Workaround for ATI driver bug
			GL_CHECK(glGenerateMipmapEXT( tex.type ));
			GL_CHECK(glDisable( tex.type ));
		}

		GL_CHECK(glBindTexture(tex.type, 0));
	}

	void GraphicsContext::DestroyTexture( u32 handle )
	{
		if(handle == 0)
			return;

		const GCTexture& tex = textures.GetRef(handle);
		GL_CHECK(glDeleteTextures(1, &tex.glObj));
		textures.Remove(handle);
	}

	void GraphicsContext::UseTexture(u32 slot, u32 handle)
	{
		const GCTexture& tex = textures.GetRef(handle);
		GL_CHECK(glActiveTexture(GL_TEXTURE0 + slot));
		GL_CHECK(glBindTexture(tex.type, tex.glObj));
	}

	// Render Buffers.
	glm::u32 GraphicsContext::CreateRenderbuffer( u32 width, u32 height, TextureFormats::List format, bool depth, u32 num_color_buffers, u32 samples /* = 0 */ )
	{
		if(num_color_buffers > GCRenderBuffer::kMaxColorAttachments)
			return 0;

		GCRenderBuffer rb;
		rb.width = width;
		rb.height = height;
		rb.samples = samples;

		// Generate framebuffers.
		GL_CHECK(glGenFramebuffers(1, &rb.fbo));
		if(samples > 0) GL_CHECK(glGenFramebuffers(1, &rb.fboMS));

		// Create and attach color buffers.
		if(num_color_buffers > 0)
		{
			for(u32 i = 0; i < num_color_buffers; ++i)
			{
				GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fbo));

				// Create the color texture.
				u32 texture_handle = CreateTexture(TextureTypes::Tex2D, rb.width, rb.height, 1, format, false, false, false, false);
				ASSERT(texture_handle != 0, "Failed to create texture");
				UploadTextureData(texture_handle, 0, 0, NULL);
				rb.colorTextures[i] = texture_handle;
				const GCTexture& texture = textures.GetRef(texture_handle);
				// Attach it.
				GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, texture.glObj, 0));

				if(samples > 0)
				{
					GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fboMS));
					GL_CHECK(glGenRenderbuffers(1, &rb.colorBuffers[i]));
					GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, rb.colorBuffers[i]));
					GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, rb.samples, texture.glFormat, rb.width, rb.height));
					GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, rb.colorBuffers[i]));
				}

				u32 buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
				GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fbo));
				GL_CHECK(glDrawBuffers(num_color_buffers, buffers));

				if(samples > 0)
				{
					GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fboMS));
					GL_CHECK(glDrawBuffers(num_color_buffers, buffers));
				}
			}
		}
		else
		{
			GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fbo));
			GL_CHECK(glDrawBuffer(GL_NONE));
			GL_CHECK(glReadBuffer(GL_NONE));

			if(samples > 0)
			{
				GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fboMS));
				GL_CHECK(glDrawBuffer(GL_NONE));
				GL_CHECK(glReadBuffer(GL_NONE));
			}
		}

		// Create and attach depth buffer.
		if(depth)
		{
			GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fbo));

			// Create the depth texture.
			u32 texture_handle = CreateTexture(TextureTypes::Tex2D, rb.width, rb.height, 1, TextureFormats::DEPTH, false, false, false, false);
			ASSERT(texture_handle != 0, "Failed to create texture");
			GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE));
			UploadTextureData(texture_handle, 0, 0, NULL);
			rb.depthTexture = texture_handle;
			const GCTexture& texture = textures.GetRef(texture_handle);
			// Attach it.
			GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture.glObj, 0));

			if(samples > 0)
			{
				GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fboMS));
				GL_CHECK(glGenRenderbuffers(1, &rb.depthBuffer));
				GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, rb.depthBuffer));
				GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, rb.samples, GL_DEPTH_COMPONENT24, rb.width, rb.height));
				GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb.depthBuffer));
			}
		}

		u32 rb_obj = renderBuffers.Add(rb);

		// Check if FrameBuffer is complete.
		bool valid = true;
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fbo));
		u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, defaultRenderbuffer));
		if(status != GL_FRAMEBUFFER_COMPLETE) 
			valid = false;

		if(samples > 0)
		{
			GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER_EXT, rb.fboMS));
			status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			glBindFramebuffer(GL_FRAMEBUFFER, defaultRenderbuffer);
			if(status != GL_FRAMEBUFFER_COMPLETE) 
				valid = false;
		}

		if( !valid )
		{
			DestroyRenderBuffer(rb_obj);
			return 0;
		}


		return rb_obj;
	}

	void GraphicsContext::DestroyRenderBuffer( u32 handle )
	{
		GCRenderBuffer rb = renderBuffers.GetRef(handle);

		if(rb.depthTexture != 0) DestroyTexture(rb.depthTexture);
		if(rb.depthBuffer != 0) GL_CHECK(glDeleteRenderbuffers(1, &rb.depthBuffer));
		rb.depthTexture = rb.depthTexture = 0;

		for(u32 i = 0; i < GCRenderBuffer::kMaxColorAttachments; ++i)
		{
			if(rb.colorTextures[i] != 0) DestroyTexture(rb.colorTextures[i]);
			if(rb.colorBuffers[i] != 0) GL_CHECK(glDeleteRenderbuffers(1, &rb.colorBuffers[i]));
			rb.colorTextures[i] = rb.colorBuffers[i] = 0;
		}

		if(rb.fbo != 0) GL_CHECK(glDeleteFramebuffers(1, &rb.fbo));
		if(rb.fboMS!= 0) GL_CHECK(glDeleteFramebuffers(1, &rb.fboMS));
		rb.fbo = rb.fboMS = 0;

		renderBuffers.Remove(handle);
	}

	// Return a texture attached to the specified render buffer (handle).
	// Use a tex_index == 32 to get the depth texture.
	u32 GraphicsContext::GetRenderbufferTexture( u32 handle, u32 tex_index )
	{
		const GCRenderBuffer& rb = renderBuffers.GetRef(handle);

		if(tex_index < GCRenderBuffer::kMaxColorAttachments)
			return rb.colorTextures[tex_index];
		else if(tex_index == 32)
			return rb.depthTexture;
		else
			return 0;
	}

	void GraphicsContext::SetRenderBuffer( u32 handle )
	{
		// TODO: Resolve renderbuffers if using multisampling (write a function for it)

		currentRenderbuffer = handle;
		if(handle == 0)
		{
			GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, defaultRenderbuffer));
			GL_CHECK(glDisable(GL_MULTISAMPLE));
		}
		else
		{
			const GCRenderBuffer& rb =renderBuffers.GetRef(handle);
			GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, rb.fboMS != 0 ? rb.fboMS : rb.fbo));
			if(rb.fboMS != 0)
				GL_CHECK(glEnable(GL_MULTISAMPLE));
			else
				GL_CHECK(glDisable(GL_MULTISAMPLE));
		}
	}

}
