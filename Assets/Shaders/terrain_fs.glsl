#version 420

layout(location = 0) out vec4 outColour;

in vec3 varying_position;
in vec3 varying_normal;
in vec2 varying_texcoords;

in vec3 view_space_position;
in vec3 view_space_normal;

vec3 sunColour	= vec3(1.0f);
vec3 lightDir = vec3(0.7f, 0.3f, -1.0f);

layout (binding = 0) uniform sampler2D normal_map;
layout (binding = 4) uniform sampler2D diffuse_map;

uniform bool refractionPass;

uniform mat4 viewMatrix;

uniform vec3 cameraPosition;

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

void main()
{
	vec3 normal0 = perturb_normal(normalize(varying_normal), cameraPosition - varying_position, varying_texcoords, 1.0f);
	//vec3 N = normalize(texture(normal_map, varying_texcoords).xyz);
	//vec3 N = normalize(varying_normal);
	vec3 N = normalize(normal0);//normalize(vec3(inverse(viewMatrix) * vec4(normal0, 0.0f)));//normalize(varying_normal);
	vec3 L = normalize(lightDir);
	float NdL = max(0.0f, dot(N, L));
	
	vec3 terrainColour = texture(diffuse_map, varying_texcoords).xyz;
	vec3 light_colour = sunColour * terrainColour * NdL;
	outColour = vec4(0.1f * sunColour + light_colour, refractionPass ? floor(0.0f - varying_position.y) / 500.0f : 1.0f);
}