#version 420

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
/*
layout (location = 2) in vec2 vertex_texture_coords;
*/

// Default uniforms. Should be in every shaders.
uniform mat4 viewProjectionMatrix; 

uniform mat4 modelMatrix;

uniform vec3 cameraPosition;

out vec3 varying_position;
out vec3 varying_normal;
out vec3 texcoords;
/*
out vec2 varying_texture_coords;
*/
void main(void)
{
	/*
	varying_position = vec3(model_xform[0] * vec4(vertex_position, 1.0));
	varying_normal = vec3(model_xform[0] * vec4(vertex_normal, 0.0));

	varying_texture_coords = vertex_texture_coords;
	*/

	varying_normal = vec3(modelMatrix * vec4(vertex_normal, 0.0));
	varying_position = vec3(modelMatrix * vec4(vertex_position, 1.0f));
	
	texcoords = varying_position;
	gl_Position = viewProjectionMatrix * modelMatrix * vec4(vertex_position, 1.0f);
}