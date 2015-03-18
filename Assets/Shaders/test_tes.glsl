	#version 420
layout(triangles, equal_spacing, cw) in;

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
	 
	mat4 modelViewProjectionMatrix = viewProjectionMatrix * modelMatrix;
	 
	in vec3 varying_position_tc[];
	in vec3 varying_normal_tc[];
	in vec2 varying_texcoords_tc[];

	in vec3 view_space_position_tc[];
	in vec3 view_space_normal_tc[];

	in vec3 reflected_vector_tc[];

	in vec4 projected_texcoords_tc[];
	
	out vec3 varying_position_te;
	out vec3 varying_normal_te;
	out vec2 varying_texcoords_te;

	out vec3 view_space_position_te;
	out vec3 view_space_normal_te;

	out vec3 reflected_vector_te;

	out vec4 projected_texcoords_te;

	
vec3 Interpolate3D(vec3 v0, vec3 v1, vec3 v2)                                                   
{                                                                                               
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;   
}     
vec2 Interpolate2D(vec2 v0, vec2 v1, vec2 v2)                                                   
{                                                                                               
    return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;   
}

    void main()
	 {
	 varying_position_te = Interpolate3D(varying_position_tc[0], varying_position_tc[1], varying_position_tc[2]);
		varying_normal_te = Interpolate3D(varying_normal_tc[0],varying_normal_tc[1],varying_normal_tc[2]);
		varying_texcoords_te = Interpolate2D(varying_texcoords_tc[0], varying_texcoords_tc[1], varying_texcoords_tc[2]);

		view_space_position_te = Interpolate3D(view_space_position_tc[0], view_space_position_tc[1], view_space_position_tc[2]);
      view_space_normal_te = Interpolate3D(view_space_normal_tc[0], view_space_normal_tc[1], view_space_normal_tc[2]);

			vec3 p0 = gl_in[0].gl_Position.xyz;
			vec3 p1 = gl_in[1].gl_Position.xyz;
			vec3 p2 = gl_in[2].gl_Position.xyz;
			vec3 tePosition = Interpolate3D(p0, p1, p2);
			gl_Position = modelViewProjectionMatrix * vec4(tePosition, 1);
    }