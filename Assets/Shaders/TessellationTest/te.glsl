#version 400

layout(triangles, equal_spacing, cw) in;
in vec3 tcPosition[];
out vec3 tePosition;
out vec3 tePatchDistance;
uniform mat4 viewProjectionMatrix; 
	uniform mat4 viewMatrix;
	uniform mat4 modelMatrix;

vec3 Interpolate3D(vec3 v0, vec3 v1, vec3 v2)                                                   
{                                                                                               
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;   
}            

void main()
{
    vec3 p0 = tcPosition[0];
    vec3 p1 = tcPosition[1];
    vec3 p2 = tcPosition[2];
    tePatchDistance = gl_TessCoord;
    tePosition = interpolate3D(p0, p1, p2);
    gl_Position = viewProjectionMatrix * modelMatrix * vec4(tePosition, 1);
}