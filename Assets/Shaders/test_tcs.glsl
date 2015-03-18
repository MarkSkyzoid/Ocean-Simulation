#version 420

layout(vertices = 3) out;

uniform mat4 viewProjectionMatrix; 
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 remappingMatrix = mat4( 	0.5f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.5f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.5f, 0.0f,
					0.5f, 0.5f, 0.5f, 1.0f );
uniform vec3 cameraPosition;
uniform float time;
uniform vec2 windDir;

uniform vec2 screenSize = vec2(1920, 1080);
uniform float lodFactor = 1.0f;

mat4 modelViewProjectionMatrix = viewProjectionMatrix * modelMatrix;

in vec3 varying_position[];
in vec3 varying_normal[];
in vec2 varying_texcoords[];

in vec3 view_space_position[];
in vec3 view_space_normal[];

in vec3 reflected_vector[];

in vec4 projected_texcoords[];

out vec3 varying_position_tc[];
out vec3 varying_normal_tc[];
out vec2 varying_texcoords_tc[];

out vec3 view_space_position_tc[];
out vec3 view_space_normal_tc[];

out vec3 reflected_vector_tc[];

out vec4 projected_texcoords_tc[];

// Helpers.

// Project from world space to device normal space
vec4 Project(vec4 vertex)
{
	vec4 result = modelViewProjectionMatrix * vertex;
	result /= result.w;
	return result;
}

// Convert from device normal to screen space.
vec2 ScreenSpace(vec4 vertex)
{
    return (clamp(vertex.xy, -1.3, 1.3)+1) * (screenSize*0.5);
}

// For LoD.
float Level(vec2 v0, vec2 v1)
{
	return clamp(distance(v0, v1) / lodFactor, 1, 64);
}

bool Offscreen(vec4 vertex)
{
    if(vertex.z < -0.5)
        return true;   
		  
    return any(
        lessThan(vertex.xy, vec2(-1.7)) ||
        greaterThan(vertex.xy, vec2(1.7))
    );  
}

void main()
{
		#define ID gl_InvocationID
		gl_out[ID].gl_Position = gl_in[ID].gl_Position;
	  
	   varying_position_tc[ID] = varying_position[ID];
		varying_normal_tc[ID] = varying_normal[ID];
		varying_texcoords_tc[ID] = varying_texcoords[ID];

		view_space_position_tc[ID] = view_space_position[ID];
      view_space_normal_tc[ID] = view_space_normal[ID];

		reflected_vector_tc[ID] = reflected_vector[ID];

		projected_texcoords_tc[ID] = projected_texcoords[ID];
	  
     if(ID == 0)
	  {
         vec4 v0 = Project(gl_in[0].gl_Position);
         vec4 v1 = Project(gl_in[1].gl_Position);
         vec4 v2 = Project(gl_in[2].gl_Position);
        // vec4 v3 = Project(gl_in[3].gl_Position);

         if(all(bvec3(
             Offscreen(v0),
             Offscreen(v1),
             Offscreen(v2)
				 )))
			{
             gl_TessLevelInner[0] = 0;
             //gl_TessLevelInner[1] = 0;
             gl_TessLevelOuter[0] = 0;
             gl_TessLevelOuter[1] = 0;
             gl_TessLevelOuter[2] = 0;
             //gl_TessLevelOuter[3] = 0;
         }
         else
			{
             // vec2 ss0 = ScreenSpace(v0);
             // vec2 ss1 = ScreenSpace(v1);
             // vec2 ss2 = ScreenSpace(v2);
             // //vec2 ss3 = ScreenSpace(v3);

             // float e0 = Level(ss1, ss2);
             // float e1 = Level(ss0, ss1);
             // float e2 = Level(ss3, ss0);
             // float e3 = Level(ss2, ss3);

             gl_TessLevelInner[0] = 3;//mix(e1, e2, 0.5);
             // gl_TessLevelInner[1] = mix(e0, e3, 0.5);
             gl_TessLevelOuter[0] = 3;//e0;
             gl_TessLevelOuter[1] = 3;//e1;
             gl_TessLevelOuter[2] = 3;//e2;
             //gl_TessLevelOuter[3] = e3;
         }
     }
 }