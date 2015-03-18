#version 420

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec2 texcoords;
/*
layout (location = 2) in vec2 vertex_texture_coords;
*/

// Default uniforms. Should be in every shaders.
uniform mat4 viewProjectionMatrix; 
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 reflectionMatrix;

uniform vec3 cameraPosition;

out vec3 varying_position;
out vec3 varying_normal;
out vec2 varying_texcoords;

out vec3 view_space_position;
out vec3 view_space_normal;

uniform vec4 clipPlaneReflection = vec4(0.0f, -1.0f, 0.0f, 0.0f);
uniform vec4 clipPlaneRefraction = vec4(0.0f, -1.0f, 0.0f, 3.0f);

void main()
{
	mat4 model_matrix = reflectionMatrix * modelMatrix;
	varying_normal = vec3(model_matrix * vec4(vertex_normal, 0.0));
	varying_position = vec3(model_matrix * vec4(vertex_position, 1.0f));
		
	varying_texcoords = texcoords;

	view_space_normal = vec3(viewMatrix * modelMatrix * vec4(vertex_normal, 0.0));
	view_space_position = vec3(viewMatrix * modelMatrix * vec4(vertex_position, 1.0f));
	
	gl_Position = viewProjectionMatrix * model_matrix * vec4(vertex_position, 1.0f);
	gl_ClipDistance[0] = dot(vec4(varying_position, 1.0f), clipPlaneReflection);
	gl_ClipDistance[1] = dot(vec4(varying_position, 1.0f), clipPlaneRefraction);
}