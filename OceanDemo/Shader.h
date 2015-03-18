#pragma once

#include "Types.h"

#include <GL\glew.h>

namespace acqua
{
	// Forward Declarations.
	class GraphicsContext;

	// Types used by GLSL uniforms
	enum UniformTypes
	{
		UT_FLOAT					= GL_FLOAT,
		UT_FLOAT_VEC2				= GL_FLOAT_VEC2,
		UT_FLOAT_VEC3				= GL_FLOAT_VEC3,
		UT_FLOAT_VEC4				= GL_FLOAT_VEC4,
		UT_DOUBLE					= GL_DOUBLE,
		UT_INT						= GL_INT,
		UT_UNSIGNED_INT				= GL_UNSIGNED_INT,
		UT_BOOL						= GL_BOOL,
		UT_FLOAT_MAT2				= GL_FLOAT_MAT2,
		UT_FLOAT_MAT3				= GL_FLOAT_MAT3,
		UT_FLOAT_MAT4				= GL_FLOAT_MAT4,
		UT_SAMPLER_1D				= GL_SAMPLER_1D,
		UT_SAMPLER_2D				= GL_SAMPLER_2D,
		UT_SAMPLER_3D				= GL_SAMPLER_3D,
		UT_SAMPLER_CUBE				= GL_SAMPLER_CUBE,

		UT_SAMPLER_2D_MULTISAMPLE	= GL_SAMPLER_2D_MULTISAMPLE,

		UT_SAMPLER_BUFFER			= GL_SAMPLER_BUFFER,
		UT_SAMPLER_2D_SHADOW		= GL_SAMPLER_2D_SHADOW,
	};

	enum ShaderType
	{
		VERTEX_SHADER					= GL_VERTEX_SHADER,
		FRAGMENT_SHADER					= GL_FRAGMENT_SHADER,
		GEOMETRY_SHADER					= GL_GEOMETRY_SHADER,
		TESSELATION_CONTROL_SHADER		= GL_TESS_CONTROL_SHADER,
		TESSELLATION_EVALUATION_SHADER	= GL_TESS_EVALUATION_SHADER,
		NULL_SHADER // just empty shader.
	};

	struct ShaderAttribute
	{
		u32 location;

		ShaderAttribute() : location(0) {}

		ShaderAttribute(u32 loc) : location(loc) {}
	};

	struct ShaderUniform
	{
		UniformTypes	type;
		u32				size;
		u32				location;

		ShaderUniform() :
			type(UT_FLOAT)
			, size(0)
			, location(0)
		{
		}

		ShaderUniform( UniformTypes s_type, u32 s_size, u32 s_location) :
			type(s_type)
			, size(s_size)
			, location(s_location)
		{}
	};

	// GLSL Shader class.
	class Shader
	{
	public:
		Shader(void);
		~Shader(void);

		bool Create(ShaderType shader_type, const char* source);
		void Destroy();

	private:
		ShaderType	type;
		u32			glObj;
		bool		compiled;

		friend class GraphicsContext;
		friend class ShaderProgram;
	};

	class ShaderProgram
	{

	public:
		typedef ACQUA_MAP<String, ShaderUniform>	UniformMap;
		typedef ACQUA_MAP<String, ShaderAttribute>	AttributeMap;

	public:
		ShaderProgram(void);
		~ShaderProgram(void);

		bool Create(u32* shader_handles, u32 num_shaders);
		void Destroy();

		UniformMap& GetUniformInterface() { return uniformsMap; }
		AttributeMap& GetAttributeInterface() { return attributesMap; }

		// Set a uniform from array of values. Might be an array or a vector or a matrix. Or an array of vectors/matrices.
		void SetUniformFromArray(const String& uniform_name, void* values, i32 num_values, bool transpose_matrix = false); 
		// Set a uniform from a single data value.
		void SetUniform(const String& uniform_name, real64 value);
	
	private:
		GraphicsContext*	graphicsContext; // Owner.
		u32					glObj;

		bool	linked;
		u32		numAttachedShaders;

		UniformMap		uniformsMap;
		AttributeMap	attributesMap;

		friend class GraphicsContext;
	};
}
