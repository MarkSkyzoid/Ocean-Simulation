#version 420

uniform vec3 cameraPosition;

layout (binding = 1) uniform samplerCube test_texture;

layout(location = 0) out vec4 outColour;

in vec3 varying_position;
in vec3 varying_normal;
in vec3 camera_space_position;

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
float R0			= 0.02037f;
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

void main(void)
{
	//outColour = vec4(0.415f, 0.698f, 0.698f, 1.0f) * (0.5f * cos(varying_position.y) + 0.5f);

	//outColour = mix(vec4(0.415f, 0.698f, 0.698f, 1.0f) * 0.3f, vec4(0.415f, 0.698f, 0.698f, 1.0f), smoothstep(-10, 10, varying_position.y));

	/*
	vec3 L = normalize(vec3(-0.5f, 0.4f, 0.0f));
	vec3 V = normalize(cameraPosition - varying_position);

	float NdL = max(0.0f, dot(normalize(varying_normal), L));

	vec2 uv = varying_position.xz * 0.007f; //uv = 0.5f + (gl_FragCoord.xy * 0.5f);

	vec3 tex_colour = texture(test_texture, reflect(V, normalize(varying_normal))).rgb;//vec3(0.415f, 0.698f, 0.698f);

	vec3 wave_col = vec3(0.415f, 0.698f, 0.698f);
	vec3 deep_col = wave_col * 0.3f;//vec3(0.03f, 0.25f, 0.25f);

	float facing = max(0.0f, dot(V, normalize(varying_normal)));

	vec3 water_col = (1.0f - facing) * (wave_col + tex_colour) + facing * (deep_col + tex_colour);

	outColour = vec4(vec3(0.2f, 0.2f, 0.2f), 1.0f) + vec4((water_col) * NdL, 1.0f);
	*/
	lightDir = normalize(lightDir);
	
	vec3 eye_vector = cameraPosition - varying_position;
	vec3 eye_vector_normalized = normalize(eye_vector);

	vec3 normal = normalize(varying_normal);

	float fresnel = FresnelTerm(normal, eye_vector_normalized);

	// Calculate refraction colour.
	vec3 refracted_ray = refract(eye_vector_normalized, normal, refractionRatio);
	vec3 refraction_colour = texture(test_texture, refracted_ray).rgb; // TODO: Use refraction map.

	// TODO: Use actual depth information.
	// ATM, use depth based on world coordinates. Use this assumption to see how colouring works.
	float depth1 = mix(0.0f, 1000.0f, smoothstep(0.0f, 5000.0f, varying_position.z));
	float depth2 = depth1;

	vec3 sun_contribution = vec3(clamp(length(sunColour) / sunScale, 0.0f, 1.0f)); // Scale the colours by the sun intensity (useful if simulating atmospheric scattering, which is a TODO).
	vec3 refraction = mix(refraction_colour, surfaceColour * sun_contribution, clamp(depth1 / visibility, 0.0f, 1.0f));
	refraction = mix(refraction, depthColour * sun_contribution, clamp(depth2 / extinction, 0.0f, 1.0f));

	float facing = max(0.0f, dot(eye_vector_normalized, vec3(0.0f, 1.0f, 0.0f)));

	// Reflections.
	// Global Reflections.
	vec3 reflection = texture(test_texture, reflect(eye_vector_normalized, normal)).rgb;
	float facing_up = dot(vec3(0.0f, 1.0f, 0.0f), normal);
	fresnel = mix(0.0f, fresnel, smoothstep(0.0f, 0.3f, facing_up)); // Avoid sampling cubemap from below (seems unnatural).

	vec3 water_colour = (1.0f - fresnel) * refraction + fresnel * reflection;//mix(refraction, reflection, fresnel);

	water_colour = (1.0f - facing) * (water_colour + surfaceColour * 0.3f) + facing * (water_colour * depthColour * 0.3f); // TODO: Compute adequate colour.

	// Specular.
	vec3 specular = vec3(0.0f);
	
	vec3 reflected_light = reflect(-lightDir, normal);
	float eye_dot_light = max(0.0f, dot(eye_vector_normalized, reflected_light));
	specular = sunColour * pow(eye_dot_light, shininess);
	/*
	vec3 mirror_eye = (2.0f * dot(eye_vector_normalized, normal) * normal - eye_vector_normalized);
	float dot_spec = clamp(dot(mirror_eye, -lightDir) * 0.5f + 0.5f, 0.0f, 1.0f);
	specular = (1.0f - fresnel) * clamp(-lightDir.y, 0.0f, 1.0f) * ((pow(dot_spec, 512.0f)) * (shininess * 1.8f + 0.2f))* sunColour;
	specular += specular * 25 * clamp(shininess - 0.05f, 0.0f, 1.0f) * sunColour;
	*/

	water_colour = water_colour + specular;

	outColour = vec4(water_colour, 1.0f);
}
