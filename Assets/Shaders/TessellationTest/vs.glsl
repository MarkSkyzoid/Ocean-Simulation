#version 400

layout (location = 0) in vec3 Position;
out vec3 vPosition;

void main()
{
    vPosition = Position.xyz;
}
