#version 420

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 3) in vec2 texcoords;
layout (location = 4) in float foamAmount;
/*
layout (location = 2) in vec2 vertex_texture_coords;
*/

// Default uniforms. Should be in every shaders.
uniform mat4 viewProjectionMatrix; 
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

uniform mat4 remappingMatrix = mat4( 	0.5f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.5f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.5f, 0.0f,
					0.5f, 0.5f, 0.5f, 1.0f );

uniform vec3 cameraPosition;

out vec3 varying_position;
out vec3 varying_normal;
out vec2 varying_texcoords;
out float varying_foam;

out vec3 view_space_position;
out vec3 view_space_normal;

out vec3 reflected_vector;

out vec4 projected_texcoords;

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

	varying_texcoords = texcoords;

	varying_foam = foamAmount;
	
	view_space_normal = vec3(viewMatrix * modelMatrix * vec4(vertex_normal, 0.0));
	view_space_position = vec3(viewMatrix * modelMatrix * vec4(vertex_position, 1.0f));

	reflected_vector = reflect(vec3(varying_position - cameraPosition), normalize(varying_normal));
	
	vec3 disturbed_pos = varying_position;
	//disturbed_pos.xz = varying_position.xz + 0.7f * varying_normal.xz; //TODO: MOVE THIS IN FRAGMENT!!
	projected_texcoords = remappingMatrix * viewProjectionMatrix * vec4(disturbed_pos, 1.0f);
	
	gl_Position = vec4(vertex_position, 1.0f);
}