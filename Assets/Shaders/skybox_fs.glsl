#version 420

layout (binding = 1) uniform samplerCube cube_texture;

layout(location = 0) out vec4 outColour;

in vec3 texcoords;

void main(void)
{
	outColour = texture (cube_texture, texcoords).rgba;
}
