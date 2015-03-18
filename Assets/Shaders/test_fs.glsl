#version 420

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

layout (binding = 0) uniform sampler2D normal_map;
layout (binding = 1) uniform samplerCube test_texture;
layout (binding = 2) uniform sampler2D	local_relfections_map;
layout (binding = 3) uniform sampler2D	refraction_map;

layout(location = 0) out vec4 outColour;

in vec3 varying_position;
in vec3 varying_normal;
in vec2 varying_texcoords;

in vec3 view_space_position;
in vec3 view_space_normal;

in vec3 reflected_vector;

in vec4 projected_texcoords;

// Colours.
// Colour of the water surface.
vec3 surfaceColour = vec3(0.0078f, 0.5176f, 0.7f);
// Colour of the water depth
vec3 depthColour = vec3(0.0039f, 0.00196f, 0.145f);
vec3 extinction = vec3(7.0f, 30.0f, 40.0f);			// Water colour extinction with depth.
// Water transparency along eye vector.
float visibility = 1.0f;

float refractionStrength = -0.1f;

// Index of refractions:
// n1 = 1.000293 (air) n2 = 1.333333 (water at 20°C / 68°F)
float refractionRatio = 0.75f; // Their ratio.

// Constants for the Fresnel term. (See: ShaderX - Advanced Water Effects)
float R0		= 0.02037f;
float OneMinusR0	= 0.97963f; 

float shininess = 128.0f;;
vec3 sunColour	= vec3(1.0f);
float sunScale	= 3.0f;

vec3 lightDir = vec3(0.7f, 0.3f, -1.0f);

float FresnelTerm(vec3 normal, vec3 eye_vector)
{
	// Uses Schlick's approximation.
	float angle = 1.0f - max(0.0f, dot(normal, eye_vector));
	float angle_pow_five = pow(angle, 5);
	float fresnel = R0 + OneMinusR0 * angle_pow_five;

	return clamp(fresnel - refractionStrength, 0.0f, 1.0f);
}

// Normal Mapping
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord, float strength )
{
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
    vec3 map = texture(normal_map, texcoord ).xyz;
    map = map * 255./127. - 128./127.;
    mat3 TBN = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * map);
}

void main(void)
{
	lightDir = normalize(lightDir);
	
	vec3 eye_vector = cameraPosition - varying_position;
	vec3 eye_vector_normalized = normalize(eye_vector);

	vec3 normal = normalize(varying_normal);

	vec2 direction = windDir;
	float wind_strength = length(direction) * 0.01f;
	vec2 offset = vec2(time * 0.004f, 0.0f); 
	vec3 normal0 = perturb_normal(view_space_normal, -normalize(view_space_position), (varying_texcoords * 1.6f + direction * time * 0.016f * wind_strength) * 1.0f, 0.01f );
	vec3 normal1 = perturb_normal(view_space_normal, -normalize(view_space_position), (varying_texcoords * 0.8f + direction * time * 0.008f * wind_strength) * 1.0f, 0.01f );
	vec3 normal2 = perturb_normal(view_space_normal, -normalize(view_space_position), (varying_texcoords * 0.4f + direction * time * 0.004f * wind_strength) * 1.0f, 0.01f );
	vec3 normal3 = perturb_normal(view_space_normal, -normalize(view_space_position), (varying_texcoords * 0.1f + direction * time * 0.002f * wind_strength) * 1.0f, 0.01f );
	vec3 view_normal = normalize(view_space_normal * 0.6f + normal0 * 0.1f + normal1 * 0.1f + normal2 * 0.1f + normal3 * 0.1f);
	//offset = vec2(0.0F, time * 0.004f); 
	//normal = perturb_normal(-normal, -eye_vector_normalized, (varying_texcoords + offset) * 10.0f, 0.01f );
	//normal = normalize(normal);
	float fresnel = FresnelTerm(view_normal, -normalize(view_space_position));

	// Calculate refraction colour.
	vec3 world_normal = normalize(vec3(inverse(viewMatrix) * vec4(view_normal, 0.0f)));
	
	vec3 disturbed_pos = varying_position;
	disturbed_pos.xz = varying_position.xz + 10.0f * world_normal.xz;
	vec4 projected_texcoords2 = remappingMatrix * viewProjectionMatrix * vec4(disturbed_pos, 1.0f);
	
	// vec3 refracted_ray = refract(normalize(view_space_position), view_normal, refractionRatio);
	// refracted_ray = normalize(vec3(inverse(viewMatrix) * vec4(normalize(refracted_ray), 0.0f)));
	vec3 refraction_colour = texture2DProj(refraction_map, projected_texcoords2).rgb;//texture(test_texture, refracted_ray).rgb; // TODO: Use refraction map.

	// TODO: Use actual depth information.
	// ATM, use depth based on world coordinates. Use this assumption to see how colouring works.
	float depth1 = texture2DProj(refraction_map, projected_texcoords2).a;//mix(0.0f, 1000.0f, smoothstep(0.0f, 5000.0f, varying_position.z));
	depth1 = depth1 >= 1.0f ? 500.0f : depth1 * 500.0f * 0.05f;
	float depth2 = depth1;

	vec3 sun_contribution = vec3(clamp(length(sunColour) / sunScale, 0.0f, 1.0f)); // Scale the colours by the sun intensity (useful if simulating atmospheric scattering, which is a TODO).
	vec3 refraction = mix(refraction_colour, surfaceColour * sun_contribution, clamp(depth1 / visibility, 0.0f, 1.0f));
	refraction = mix(refraction, depthColour * sun_contribution, clamp(depth2 / extinction, 0.0f, 1.0f));

	float facing = max(0.0f, dot(eye_vector_normalized, vec3(0.0f, 1.0f, 0.0f)));

	// Reflections.
	// Global Reflections.
	vec3 reflected_vector2 = normalize(vec3(inverse(viewMatrix) * vec4(reflect(normalize(view_space_position), view_normal), 0.0)));
	vec3 reflection = texture(test_texture, reflected_vector2).rgb;
	//float facing_up = dot(vec3(0.0f, 1.0f, 0.0f), reflected_vector);
	//fresnel = mix(0.0f, fresnel, smoothstep(0.0f, 0.0f, facing_up)); // Avoid sampling cubemap from below (seems unnatural).
	
	// Local Reflections.
	vec4 local_reflection_colour = texture2DProj(local_relfections_map, projected_texcoords2).rgba;
	
	reflection = mix(reflection, local_reflection_colour.rgb, local_reflection_colour.a);
	
	vec3 water_colour = (1.0f - fresnel) * refraction + fresnel * reflection;//mix(refraction, reflection, fresnel);

	//water_colour = (1.0f - facing) * (water_colour + surfaceColour * 0.3f) + facing * (water_colour * depthColour * 0.3f); // TODO: Compute adequate colour.

	// Specular.
	lightDir = vec3(viewMatrix * vec4(lightDir, 0.0));
	vec3 specular = vec3(0.0f);

	vec3 reflected_light = reflect(-lightDir, view_normal);
	float eye_dot_light = max(0.0f, dot(eye_vector_normalized, reflected_light));
	specular = sunColour * pow(eye_dot_light, shininess);
	/*
	vec3 mirror_eye = (2.0f * dot(eye_vector_normalized, normal) * normal - eye_vector_normalized);
	float dot_spec = clamp(dot(mirror_eye, -lightDir) * 0.5f + 0.5f, 0.0f, 1.0f);
	specular = (1.0f - fresnel) * clamp(-lightDir.y, 0.0f, 1.0f) * ((pow(dot_spec, 512.0f)) * (shininess * 1.8f + 0.2f))* sunColour;
	specular += specular * 25 * clamp(shininess - 0.05f, 0.0f, 1.0f) * sunColour;
	*/

	float foam = 0.0f;
	//if(varying_position.y - 2.0f > 0.001f)
		//foam = smoothstep(0.0f, 15.0f, varying_position.y - 15.0f);

	water_colour = clamp(water_colour + max(specular, foam * sunColour), 0.0f, 1.0f);

	//water_colour = texture(normal_map, varying_position.xz).xyz;

	outColour = vec4(water_colour, 1.0f);
}
