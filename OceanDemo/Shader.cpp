#include "Shader.h"

#include "GraphicsContext.h"

#include "DebugUtil.h"
#include "GLUtil.h"

#include <cstdio> // TODO: Wrap headers in pch. And IO calls into a specific header.

namespace acqua
{
	Shader::Shader(void) :
		type(NULL_SHADER)
		, glObj(0)
		, compiled(false)
	{
	}


	Shader::~Shader(void)
	{
	}

	bool Shader::Create(ShaderType shader_type, const char* source)
	{
		bool result = true;
		type = shader_type;
		GL_CHECK(glObj = glCreateShader(type));

		if(glObj != 0)
		{
			GL_CHECK(glShaderSource(glObj, 1, (const GLchar**) &source, NULL));
			GL_CHECK(glCompileShader(glObj));

			GLint shader_compiled = 0;
			GL_CHECK(glGetShaderiv(glObj, GL_COMPILE_STATUS, &shader_compiled));
			compiled = shader_compiled > 0;

			if(!compiled)
			{
				char error_log[1024];
				GL_CHECK(glGetShaderInfoLog(glObj, sizeof(error_log), NULL, error_log));
				printf("Failed to compile shader %d: %s\n", shader_compiled, error_log);

				result = false;

				Destroy();
			}
		}
		else
		{
			result = false;
		}

		return result;
	}

	void Shader::Destroy(void)
	{
		GL_CHECK(glDeleteShader(glObj));
	}

	ShaderProgram::ShaderProgram(void) :
		graphicsContext(NULL)
		, glObj(0)
		, linked(0)
		, numAttachedShaders(0)
	{
	}

	ShaderProgram::~ShaderProgram()
	{
	}

	bool ShaderProgram::Create(u32* shader_handles, u32 num_shaders)
	{	
		if(num_shaders < 1)
		{
			ASSERT(0, "Not enough shaders to attach to the program.");
			return false;
		}

		if(graphicsContext == NULL)
		{
			ASSERT(0, "There is no Graphics Context. This should never happen.");
			return false;
		}

		// Create the program
		GL_CHECK(glObj = glCreateProgram());

		// TODO: Attaching shaders should go elsewhere.
		// Attach Shaders.
		for(u32 i = 0; i < num_shaders; ++i)
		{
			const Shader& shader = graphicsContext->GetShader(shader_handles[i]);
			GL_CHECK(glAttachShader(glObj, shader.glObj));
			++numAttachedShaders;
		}

		// TODO: Linking should go into a separate function.
		// Link program.
		GLint program_linked = 0;
		GL_CHECK(glLinkProgram(glObj));
		
		// Check linking results and output linking errors (if any).
		GL_CHECK(glGetProgramiv(glObj, GL_LINK_STATUS, &program_linked));
		linked = program_linked > 0;

		if(!linked)
		{
			char error_log[1024];
			GL_CHECK(glGetProgramInfoLog(glObj, sizeof(error_log), NULL, error_log));
			printf("Failed to link shader program %d: %s\n", program_linked, error_log);

			Destroy();

			return false;
		}

		// Shader reflection. Collect and store uniforms.
		// NOTE: This last step (or all of them) should MAYBE done separately.
		// This is to avoid copying the uniform map when the shader program gets added to the resurces in the GraphicsContext instance.
		// However, as this step will most likely be done at startup that should be fine.

		// TODO: Move the following into a separate function.
		if(!linked)
		{
			ASSERT(0, "In order to perform shader reflection the program must be linked.");
			return false;
		}

		GLint active_attributes;
		GLint active_uniforms;

		GL_CHECK(glGetProgramiv(glObj, GL_ACTIVE_ATTRIBUTES, &active_attributes));
		GL_CHECK(glGetProgramiv(glObj, GL_ACTIVE_UNIFORMS, &active_uniforms));

		printf("attributes: %d \n", active_attributes);
		printf("uniforms: %d \n", active_uniforms);

		for (int i = 0; i < active_attributes; ++i)
		{
			GLint size;
			GLenum type;
			char name[256];

			GL_CHECK(glGetActiveAttrib(glObj, i, 256, NULL, &size, &type, name));
			GLint location = glGetAttribLocation(glObj, name);

			printf("Attribute %s (size: %d) at location %d \n", name, size, location);

			attributesMap[name] = ShaderAttribute(location);
		}

		for (int i = 0; i < active_uniforms; ++i)
		{
			GLint size;
            GLenum type;
            char name[256];

            GL_CHECK(glGetActiveUniform(glObj, i, 256, NULL, &size, &type, name));
            GLint location = glGetUniformLocation(glObj, name);

            printf("Uniform %s (size: %d) at location %d \n", name, size, location);

            std::string uniform_name = name;

            if (uniform_name.length() > 3 && uniform_name.substr(uniform_name.length()-3) == "[0]")
            {
                uniform_name = uniform_name.substr(0, uniform_name.length()-3);

                printf("Uniform array name %s replaced with %s! \n", name, uniform_name.c_str());
            }

			uniformsMap[uniform_name] = ShaderUniform(static_cast<UniformTypes>(type), size, location);
		}

		return true;
	}

	void ShaderProgram::Destroy()
	{
		GL_CHECK(glDeleteProgram(glObj));
	}

	
	void ShaderProgram::SetUniformFromArray(const String& uniform_name, void* values, i32 num_values, bool transpose_matrix /*= false*/)
	{
		UniformMap::iterator uniform_it = uniformsMap.find(uniform_name);  
		if(uniform_it != uniformsMap.end())
		{
			ShaderUniform& uniform = uniform_it->second;
			
			GLint location = uniform.location;
			UniformTypes type = uniform.type;
			GLint size = num_values == -1 ? uniform.size : num_values;

			switch(type)
			{
			case UT_FLOAT:
				GL_CHECK(glUniform1fv(location, size, (GLfloat*) values));
				break;
			case UT_FLOAT_VEC2:
				GL_CHECK(glUniform2fv(location, size, (GLfloat*) values));
				break;
			case UT_FLOAT_VEC3:
				GL_CHECK(glUniform3fv(location, size, (GLfloat*) values));
				break;
			case UT_FLOAT_VEC4:
				GL_CHECK(glUniform4fv(location, size, (GLfloat*) values));
				break;
			case UT_DOUBLE:
				GL_CHECK(glUniform1dv(location, size, (GLdouble*) values));
				break;
			case UT_INT:
			case UT_BOOL:
			case UT_SAMPLER_1D:
            case UT_SAMPLER_2D:
            case UT_SAMPLER_3D:
            case UT_SAMPLER_CUBE:
            case UT_SAMPLER_2D_MULTISAMPLE:
            case UT_SAMPLER_BUFFER:
            case UT_SAMPLER_2D_SHADOW:
				GL_CHECK(glUniform1iv(location, size, (GLint*) values));
				break;
			case UT_UNSIGNED_INT:
				GL_CHECK(glUniform1uiv(location, size, (GLuint*) values));
				break;
			case UT_FLOAT_MAT2:
				GL_CHECK(glUniformMatrix2fv(location, size, transpose_matrix, (GLfloat*) values));
				break;
			case UT_FLOAT_MAT3:
				GL_CHECK(glUniformMatrix3fv(location, size, transpose_matrix, (GLfloat*) values));
				break;
			case UT_FLOAT_MAT4:
				GL_CHECK(glUniformMatrix4fv(location, size, transpose_matrix, (GLfloat*) values));
				break;
			default:
				ASSERT(0, "Unknown uniform type.");
				break;
			}
		}
		else
		{
			//ASSERT(0, "Uniform not found");
			//printf("Uniform %s not found\n", uniform_name);
		}
	}

	void ShaderProgram::SetUniform(const String& uniform_name, real64 value)
	{
		UniformMap::iterator uniform_it = uniformsMap.find(uniform_name);  
		if(uniform_it != uniformsMap.end())
		{
			ShaderUniform& uniform = uniform_it->second;
			
			GLint location = uniform.location;
			UniformTypes type = uniform.type;

			switch(type)
			{
			case UT_FLOAT:
				GL_CHECK(glUniform1f(location, (GLfloat) value));
				break;
			case UT_DOUBLE:
				GL_CHECK(glUniform1d(location, (GLdouble) value));
				break;
			case UT_INT:
			case UT_BOOL:
			case UT_SAMPLER_1D:
            case UT_SAMPLER_2D:
            case UT_SAMPLER_3D:
            case UT_SAMPLER_CUBE:
            case UT_SAMPLER_2D_MULTISAMPLE:
            case UT_SAMPLER_BUFFER:
            case UT_SAMPLER_2D_SHADOW:
				GL_CHECK(glUniform1i(location, (GLint) value));
				break;
			case UT_UNSIGNED_INT:
				GL_CHECK(glUniform1ui(location, (GLuint) value));
				break;
			case UT_FLOAT_VEC2:
			case UT_FLOAT_VEC3:
			case UT_FLOAT_VEC4:
			case UT_FLOAT_MAT2:
			case UT_FLOAT_MAT3:
			case UT_FLOAT_MAT4:
				ASSERT(0, "Trying to assign a single value to a vector/matrix type");
				printf("Trying to assign a single value to uniform %s of vector/matrix type\n", uniform_name.c_str());
				break;
			default:
				ASSERT(0, "Unknown uniform type.");
				break;
			}
		}
		else
		{
			//ASSERT(0, "Uniform not found");
			//printf("Uniform %s not found\n", uniform_name);
		}
	}
}
