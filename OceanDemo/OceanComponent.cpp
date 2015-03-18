#include "OceanComponent.h"

#include "GameObject.h"
#include "GraphicsContext.h"
#include "Math.h"

#include <random>
#include <cmath>
#include "SFML\Graphics\Image.hpp"

#define SEGMENT_WIDTH 10.0f
#define GRID_MULTIPLIER 5

namespace acqua
{
	// utilities.
	template <typename T> static inline T Sqr(T x) { return x*x; }
	template <typename T> static inline T Lerp(T a, T b, T t) { return a + (b - a) * t; }
	template <typename T> static inline T Catrom(T p0, T p1, T p2, T p3, T t)
	{
		return	0.5 * ((2 * p1) +
				(-p0 + p2) * t +
				(2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
				(-p0 + 3 * p1- 3 * p2 + p3) * t * t * t);
	}

	OceanComponent::OceanComponent( void ) : Component(CT_OCEANCOMPONENT)
		, vertices(NULL)
		, vertexCount(0)
		, indices(NULL)
		, indexCount(0)
		, simulationTime(0.0f)
		, M(128)
		, N(128)
		, V(2.0f)
		, L( V * V / k_Gravity )
		, l(2.0f)
		, A(1.0f)
		, W(4.0f)
		, WX(cos(W)), WZ(-sin(W))
		, windAlignment(2.0f)
		, dampReflections(0.5f)
		, depth(200.0f)
		, chopAmount(0.5f)
		, foamSlopeRatio(0.07f)
		, foamFader(0.1f)
	{
		seed = time(NULL);

		//TODO: Look up references for this values.
		const Real dx = 1.0f;
		const Real dz = 1.0f;
		LX = M * dx;
		LZ = N * dz;
	}

	OceanComponent::~OceanComponent( void )
	{
		fftwf_destroy_plan(displacementYPlan);
		fftwf_destroy_plan(displacementXPlan);
		fftwf_destroy_plan(displacementZPlan);

		fftwf_destroy_plan(normalXPlan);
		fftwf_destroy_plan(normalZPlan);
	}

	bool OceanComponent::Init( GameObject* o )
	{
		bool result = Component::Init(o);

		// Create vertex definition.
		VertexLayoutAttrib vertex_attribs[] = 
		{
			{3, 0},
			{3, sizeof(glm::vec3)},
			{3, 2 * sizeof(glm::vec3)},
			{2, 3 * sizeof(glm::vec3)},
			{1, 3 * sizeof(glm::vec3) + sizeof(glm::vec2)}
		};
		u32 num_vertex_attribs = sizeof(vertex_attribs) / sizeof(VertexLayoutAttrib);
		
		ResetOcean(OceanSettings());

#pragma region OldMeshCode
		//const u32	N_plus_1 = (N * GRID_MULTIPLIER) + 1;

		//vertices	= new VertexOcean[N_plus_1 * N_plus_1];
		//indices		= new u32[N_plus_1 * N_plus_1 * 10];

		//u32 index = 0;
		//vertexCount = 0;

		//for(u32 m_prime = 0; m_prime < N_plus_1; ++m_prime)
		//{
		//	for(u32 n_prime = 0; n_prime < N_plus_1; ++n_prime)
		//	{
		//		index = m_prime * N_plus_1 + n_prime;

		//		float x = n_prime * SEGMENT_WIDTH; //(n_prime - N / 2.0f) * SEGMENT_WIDTH / N;
		//		float z = m_prime * SEGMENT_WIDTH; //(m_prime - N / 2.0f) * SEGMENT_WIDTH / N;

		//		vertices[index].originalPosition = vertices[index].position = glm::vec3(x, 0.0f, z);
		//		vertices[index].normal = glm::vec3(0.0f, 1.0f, 0.0f);

		//		float u = n_prime  / static_cast<float>(N_plus_1);
		//		float v = m_prime / static_cast<float>(N_plus_1);

		//		vertices[index].texcoords = glm::vec2(u, v);

		//		++vertexCount;
		//	}
		//}

		//// generate indices.
		//indexCount = 0;
		//for(u32 m_prime = 0; m_prime < N * GRID_MULTIPLIER - 1; ++m_prime)
		//{
		//	for(u32 n_prime = 0; n_prime < N * GRID_MULTIPLIER - 1; ++n_prime)
		//	{
		//		index = m_prime * N_plus_1 + n_prime;

		//		indices[indexCount++] = index;
		//		indices[indexCount++] = index + N_plus_1;
		//		indices[indexCount++] = index + 1;

		//		indices[indexCount++] = index + N_plus_1;
		//		indices[indexCount++] = index + N_plus_1 + 1;
		//		indices[indexCount++] = index + 1;
		//	}
		//}
#pragma endregion

		u32 map_width  = M * GRID_MULTIPLIER;
		u32 map_height = N * GRID_MULTIPLIER;

		float terrain_width = SEGMENT_WIDTH * (map_width - 1);
		float terrain_height = SEGMENT_WIDTH * (map_height - 1);

		float half_terrain_width = terrain_width * 0.5f;
		float half_terrain_height = terrain_height * 0.5f;

		vertices = new VertexOcean[map_width * map_height];

		for(u32 j = 0; j < map_height; ++j)
		{
			for(u32 i = 0; i < map_width; ++i)
			{
				u32 index = i + (j * map_width);
				// Generate geometry information.
				// Texture coordinates.
				float u = i / static_cast<float>(map_width - 1);
				float v = j / static_cast<float>(map_height - 1);

				float x = (u * terrain_width) - half_terrain_width;
				float y = 0.0f;
				float z = (v * terrain_height) - half_terrain_height;

				vertices[index].originalPosition = vertices[index].position = glm::vec3(x, y, z);
				vertices[index].normal = glm::vec3(0.0f, 1.0f, 0.0f);
				
				u = i / (float)(map_width - 1);//(i % M) / static_cast<float>(N - 1);
				v = j / (float)(map_height - 1);//(j % N) / static_cast<float>(M - 1);
				vertices[index].texcoords = glm::vec2(u, v);

				vertices[index].foamAmount = 0.0f;

				++vertexCount;
			}
		}

		// Generate indices.
		const u32 num_triangles = (map_width - 1) * (map_height - 1) * 2;
		indices = new u32[num_triangles * 3];

		u32 index = 0;
		for(u32 j = 0; j < map_height - 1; ++j)
		{
			for(u32 i = 0; i < map_width - 1; ++i)
			{
				u32 vertex_index =  i + (j * map_width);
				indices[index++] = vertex_index;
				indices[index++] = vertex_index + map_width;
				indices[index++] = vertex_index + 1;

				indices[index++] = vertex_index + map_width;
				indices[index++] = vertex_index + map_width + 1;
				indices[index++] = vertex_index + 1;
			}
		}
		indexCount = index - 1;

		// We need geometry.
		geometry = std::make_shared<Geometry>();
		result &= geometry->Load(o->GetScene().GetGraphicsContext(), vertices, vertexCount, indices, indexCount, vertex_attribs, num_vertex_attribs);

		result &= o->AddComponent("GeometryRenderer");
		GeometryRenderer* geometry_render = o->GetComponent<GeometryRenderer>();
		if(geometry_render == NULL)
			return false;

		geometry_render->SetGeometry(geometry);

		return result;
	}

	void OceanComponent::FixedUpdate( float fixed_delta_time )
	{

	}

	void OceanComponent::Update( float delta_time )
	{
		SimulateOceanFFT(simulationTime, 1.0f / (GRID_MULTIPLIER * SEGMENT_WIDTH));

		geometry->UpdateVertexData(vertices, 0);

		simulationTime += delta_time;
	}

	void OceanComponent::Draw( float delta_time )
	{

	}

	acqua::Real OceanComponent::Ph(Real kx_,Real kz_ ) const
	{
		Real k2 = kx_*kx_ + kz_*kz_;

		if (k2 == 0.0)
		{
			return 0.0; // no DC component
		}

		// damp out the waves going in the direction opposite the wind
		float tmp = (WX * kx_  + WZ * kz_)/sqrt(k2);
		if (tmp < 0) 
		{
			tmp *= dampReflections;
		}

		return A * exp( -1.0f / (k2*Sqr(L))) * exp(-k2 * Sqr(l)) * pow(fabs(tmp),windAlignment) / (k2*k2);
	}

	void OceanComponent::SimulateOceanFFT( float t, float scale )
	{
		// Compute a new hTilda.
		for(int i = 0; i < M; ++i)
		{
			// Note the <= _N/2 here. See the fftw docs about
			// the mechanics of the complex->real fft storage
			for(int j = 0; j <= N / 2; ++j)
			{
				Real omega_k = Omega(k(i,j));
				hTilda(i, j) =	h0(i, j) * exp(ComplexReal(0, omega_k * t)) +
								std::conj(h0Minus(i, j)) * exp(ComplexReal(0, -omega_k * t));
			}
		}


		FFTIn = scale * hTilda;

		// Get y displacement_img.
		fftwf_execute(displacementYPlan);

		// x displacement_img
		for (int i = 0 ; i  < M ; ++i)
		{   
			for (int j  = 0 ; j  <= N / 2 ; ++j)
			{     
				FFTIn(i,j) = -scale * chopAmount * k_Minus_i * 
					hTilda(i,j) * (k(i,j) == 0.0 ? ComplexReal(0,0) : kx(i) / k(i,j)) ;   
			}
		}
		fftwf_execute(displacementXPlan);

		// z displacement_img
		for (int i = 0 ; i  < M ; ++i)
		{   
			for (int j  = 0 ; j  <= N / 2 ; ++j) 
			{                    
				FFTIn(i,j) = -scale * chopAmount * k_Minus_i * 
					hTilda(i,j) * (k(i,j) == 0.0 ? ComplexReal(0,0) : kz(j) / k(i,j)) ;
			}
		} 
		fftwf_execute(displacementZPlan);


		// Normals
		/*for(int i = 0; i < M; ++i)
		{
			for(int j = 0; j < N / 2; ++j)
			{
				FFTIn(i,j) = -k_Plus_i * hTilda(i, j) * kx(i);
			}
		}

		fftwf_execute(normalXPlan);


		for(int i = 0; i < M; ++i)
		{
			for(int j = 0; j < N / 2; ++j)
			{
				FFTIn(i,j) = -k_Plus_i * hTilda(i, j) * kz(i);
			}
		}

		fftwf_execute(normalZPlan);*/
/*
		sf::Image displacement_img;
		displacement_img.create(M * GRID_MULTIPLIER, N * GRID_MULTIPLIER);*/

		int x = 0;
		int y = 0;
		u32 grid_width = M * GRID_MULTIPLIER;
		for(int i = 0; i < M; ++i)
		{
			for(int j = 0; j < N; ++j)
			{
				u32 index = i + j * grid_width;
				normalArray(i, j) = glm::normalize(vertices[index].normal);
			}
		}
		for(int i = 0; i < M; ++i)
		{
			for(int j = 0; j < N; ++j)
			{
				u32 index = i + j * grid_width;

				u32 index_plus_width = index + grid_width;
				u32 index_plus_one = index + 1;

				int i_plus_one = (i + 1);
				i_plus_one = i_plus_one >= M ? 0 : i_plus_one;
				int j_plus_one = (j + 1);
				j_plus_one = j_plus_one >= N ? 0 : j_plus_one;

				glm::vec3 v0 = vertices[index].originalPosition + glm::vec3(displacementX(i,j) * 0.8f, displacementY(i,j), 0.8f * displacementZ(i,j));
				glm::vec3 v1 = vertices[index_plus_width].originalPosition + glm::vec3(0.8f * displacementX(i,j_plus_one), displacementY(i,j_plus_one), 0.8f * displacementZ(i,j_plus_one));
				glm::vec3 v2 = vertices[index_plus_one].originalPosition + glm::vec3(0.8f * displacementX(i_plus_one,j), displacementY(i_plus_one,j), 0.8f *  displacementZ(i_plus_one,j));
				glm::vec3 normal = glm::normalize( glm::cross( v1 - v0, v2 - v0 ) );

				normalArray(i, j) += normal;
				normalArray(i, j_plus_one) += normal;
				normalArray(i_plus_one, j) += normal;

				//second tri.
				glm::vec3 v3 = vertices[index_plus_width + 1].originalPosition + glm::vec3(0.8f * displacementX(i_plus_one,j_plus_one), displacementY(i_plus_one, j_plus_one), 0.8f * displacementZ(i_plus_one, j_plus_one));
				normal = glm::normalize( glm::cross( v1 - v3, v1 - v2 ) );

				normalArray(i, j_plus_one) += normal;
				normalArray(i_plus_one, j) += normal;
				normalArray(i_plus_one, j_plus_one) += normal;
			}
		}


		for(int i = 0; i < M * GRID_MULTIPLIER; ++i)
		{
			for(int j = 0; j < N * GRID_MULTIPLIER; ++j)
			{
				int x = i % M;
				int y = j % N;
				u32 index = i + j * M * GRID_MULTIPLIER;
				vertices[index].position = vertices[index].originalPosition + glm::vec3(0.8f * displacementX(x, y), displacementY(x, y), 0.8f * displacementZ(x, y));
				
				vertices[index].normal.y = 1.0f/scale;
				vertices[index].normal = normalArray(x, y);//glm::normalize(vertices[index].normal);
				
				int x_minus_1 = (x - 1) % N;
				int y_minus_1 = (y - 1) % M;
				
				int x_plus_1 = (x + 1) % N;
				int y_plus_1 = (y + 1) % M;

				float foam_add = -foamFader;
				//if(x_minus_1 >= 0 || y_minus_1 >= 0)
				{
					float jxx = scale * (displacementX(x, y) - displacementX(x_minus_1, y));
					//float jxz = scale * (displacementX(x, y) - displacementX(x, y_minus_1));
					//float jzx = scale * (displacementZ(x, y) - displacementZ(x_minus_1, y));
					float jzz = scale * (displacementZ(x, y) - displacementZ(x, y_minus_1));

					

					float j = jxx < jzz ? jxx : jzz;
					if(j < -foamSlopeRatio)
					{
						foam_add *= -1;
					}
				}
				vertices[index].foamAmount = glm::clamp(vertices[index].foamAmount + foam_add, 0.0f, 1.0f);

				/*glm::vec2 foam_vec = glm::vec2(-normalX(x, y), -normalZ(x, y));
				float foam =  foam_vec.x > foam_vec.y ? foam_vec.x : foam_vec.y;
				float foam_add = foam > 100.0f ? +0.001f : -0.001f;
				vertices[index].foamAmount = glm::clamp(vertices[index].foamAmount + foam_add, 0.0f, 1.0f);*/

				//float height = displacementY(x, y) / A;
				//sf::Uint8 height_c = static_cast<sf::Uint8>(height * 255);
				//sf::Color c(height_c,height_c,height_c,255);
				/*sf::Color c(static_cast<sf::Uint8>((vertices[index].normal.x * 255)),
					static_cast<sf::Uint8>((vertices[index].normal.y * 255)),
					static_cast<sf::Uint8>((vertices[index].normal.z * 255)));
				*///displacement_img.setPixel(i,j, c);
			}
		}

		//int x = 0;
		//int y = 0;
		//u32 grid_width = M * GRID_MULTIPLIER;
		//for(int i = 0; i < M; ++i)
		//{
		//	for(int j = 0; j < N; ++j)
		//	{
		//		u32 index = i + j * grid_width;

		//		u32 index_plus_width = index + grid_width;
		//		u32 index_plus_one = index + 1;

		//		// first triangle.
		//		VertexOcean& v0 = vertices[index];
		//		VertexOcean& v1 = vertices[index_plus_width];
		//		VertexOcean& v2 = vertices[index_plus_one];

		//		glm::vec3 normal = glm::normalize( glm::cross( v1.position - v0.position, v2.position - v0.position ) );

		//		v0.normal += normal;
		//		v1.normal += normal;
		//		v2.normal += normal;

		//		//second tri.
		//		VertexOcean& v3 = vertices[index_plus_width + 1];
		//		normal = glm::normalize( glm::cross( v1.position - v3.position, v1.position - v2.position ) );

		//		v1.normal += normal;
		//		v2.normal += normal;
		//		v3.normal += normal;
		//	}
		//}

		
		// Generate normals.
		/*for ( unsigned int i = 0; i < indexCount; i += 3 )
		{
			glm::vec3 v0 = vertices[ indices[i + 0] ].position;
			glm::vec3 v1 = vertices[ indices[i + 1] ].position;
			glm::vec3 v2 = vertices[ indices[i + 2] ].position;

			glm::vec3 normal = glm::normalize( glm::cross( v1 - v0, v2 - v0 ) );

			vertices[ indices[i + 0] ].normal += normal;
			vertices[ indices[i + 1] ].normal += normal;
			vertices[ indices[i + 2] ].normal += normal;
		}*/
		
	}

	void OceanComponent::ResetOcean( const OceanSettings& settings )
	{

		V = settings.V;
		L = V * V / k_Gravity;
		l = settings.l;
		A = settings.A;
		W = settings.W * 0.0174532925f; // Convert to radians.
		WX = cos(W); 
		WZ = -sin(W);
		windAlignment = settings.windAlignment;
		dampReflections = settings.dampReflections;
		depth = settings.depth;
		chopAmount = settings.chopAmount;

		foamFader = settings.foamFader;
		foamSlopeRatio = settings.foamSlopeRatio;

		// FFTW Inputs allocation.
		FFTIn.resize(M, 1 + N / 2);
		hTilda.resize(M, 1 + N / 2);

		// Initialize FFTW plans.
		displacementY.resize(M, N);
		displacementX.resize(M, N);
		displacementZ.resize(M, N);

		normalX.resize(M, N);
		normalZ.resize(M, N);
		normalArray.resize(M, N);

		fftwf_destroy_plan(displacementYPlan);
		fftwf_destroy_plan(displacementXPlan);
		fftwf_destroy_plan(displacementZPlan);

		displacementYPlan = fftwf_plan_dft_c2r_2d(M, N, reinterpret_cast<fftwf_complex*>(FFTIn.data()), reinterpret_cast<Real*>(displacementY.data()), FFTW_ESTIMATE);
		displacementXPlan = fftwf_plan_dft_c2r_2d(M,N, reinterpret_cast<fftwf_complex*>(FFTIn.data()), reinterpret_cast<Real*>(displacementX.data()), FFTW_ESTIMATE);
		displacementZPlan = fftwf_plan_dft_c2r_2d(M,N, reinterpret_cast<fftwf_complex*>(FFTIn.data()), reinterpret_cast<Real*>(displacementZ.data()), FFTW_ESTIMATE);

		fftwf_destroy_plan(normalXPlan);
		fftwf_destroy_plan(normalZPlan);

		normalXPlan = fftwf_plan_dft_c2r_2d(M,N, reinterpret_cast<fftwf_complex*>(FFTIn.data()), reinterpret_cast<Real*>(normalX.data()), FFTW_ESTIMATE);
		normalZPlan = fftwf_plan_dft_c2r_2d(M,N, reinterpret_cast<fftwf_complex*>(FFTIn.data()), reinterpret_cast<Real*>(normalZ.data()), FFTW_ESTIMATE);

		// Initialize matrices needed.
		k.resize(M, 1 + N / 2);
		h0.resize(M, N);
		h0Minus.resize(M, N);
		kx.resize(M);
		kz.resize(N);

		// Pre-compute k vectors (directions).
		// +ve components
		for(int i = 0; i <= M / 2; ++i)
		{
			kx(i) = 2.0f * k_Pi * i / LX;
		}

		// -ve components
		for(int i = M - 1, ii = 0; i > M / 2; --i, ++ii)
		{
			kx(i) = -2.0f * k_Pi * ii / LX;
		}

		// +ve components
		for(int i = 0; i <= N / 2; ++i)
		{
			kz(i) = 2.0f * k_Pi * i / LZ;
		}

		// -ve components
		for(int i = N - 1, ii = 0; i > N / 2; --i, ++ii)
		{
			kz(i) = -2.0f * k_Pi * ii / LZ;
		}

		// Pre-compute the k matrix (magnitude of k vectors)
		for(int i = 0; i < M; ++i)
		{
			for(int j = 0; j <= N / 2; ++j) // Note <= _N/2 here, see the fftw notes about complex->real fft storage.
			{
				k(i, j) = sqrt(kx(i) * kx(i) + kz(j) * kz(j));
			}
		}

		// DEBUG: Want to look at the wavelengths of the components ?
		//for (int i = 0 ; i < M ; ++i)
		//std::cout << "kx[" << i << "]=" << kx(i) << " wl=" << Wavelength(kx(i)) << " factor = " << Ph(kx(i),kx(i)) << std::endl ;

		Imath::Rand32 rand(seed);

		// Pre-compute hTilda0.
		for(int i = 0; i < M; ++i)
		{
			for(int j = 0; j < N; ++j)
			{
				Real r1 = Imath::gaussRand(rand);
				Real r2 = Imath::gaussRand(rand);
				ComplexReal h = ComplexReal(r1, r2) * float(sqrt(Ph(kx(i), kz(j)) / 2.0f));
				h0(i, j) = h;
				h0Minus(i, j) = ComplexReal(r1, r2) * float(sqrt(Ph(-kx(i), -kz(j)) / 2.0f));

				//std::cout << "h0("<<i<<", "<<j<<") = " << h0(i,j).real() << " - " << h0(i,j).imag() << std::endl;
			}
		}
	}

}

/*
float uniformRandomVariable();
complex gaussianRandomVariable();

float uniformRandomVariable() {
	return (float)rand()/RAND_MAX;
}

complex gaussianRandomVariable() {
	float x1, x2, w;
	do {
		x1 = 2.f * uniformRandomVariable() - 1.f;
		x2 = 2.f * uniformRandomVariable() - 1.f;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.f );
	w = sqrt((-2.f * log(w)) / w);
	return complex(x1 * w, x2 * w);
}

blitz::Array<float, 2> test_array;

namespace acqua
{
	OceanComponent::OceanComponent(void) : Component(CT_OCEANCOMPONENT)
		,simulationTime(0.0f)
		,vertices(NULL)
		,vertexCount(0)
		,indices(NULL)
		,indexCount(0)
		,gravity(9.8f)
		,N(64)
		,A(1.0f)
		,length(1.0f)
		,fft(NULL)
		,hTilde(NULL)
		,hTildeX(NULL)
		,hTildeZ(NULL)
	{
		test_array.resize(N, N);
	}


	OceanComponent::~OceanComponent(void)
	{
		delete[] vertices;
		delete[] indices;

		//fftw_destroy_plan(fftwPlan);
		//fftw_free(hTilde);


		delete[] hTilde;
		delete[] hTildeX;
		delete[] hTildeZ;
		delete fft;
	}

	bool OceanComponent::Init( GameObject* o )
	{
		bool result = Component::Init(o);
		// Create vertex definition.
		VertexLayoutAttrib vertex_attribs[] = 
		{
			{3, 0},
			{3, sizeof(glm::vec3)},
			{3, 2 * sizeof(glm::vec3)},
			{3, 3 * sizeof(glm::vec3)},
			{3, 4 * sizeof(glm::vec3)}
		};
		u32 num_vertex_attribs = sizeof(vertex_attribs) / sizeof(VertexLayoutAttrib); 

		// Generate ocean plane.
			
		// Set ocean parameters
		// TODO: Make this settable and gettable.
		gravity = 3.8f;
		N = 128;
		length = 128.0f;
		wind = glm::vec2(25.0f,5.0f);
		A = 0.0015f; //0.0005f;

		const u32	N_plus_1 = N + 1;
		
		
		hTilde		=  new complex[N*N];//(complex*) fftw_malloc(sizeof(complex) * N * N); //new complex[N*N];
		hTildeX		=  new complex[N*N];
		hTildeZ		=  new complex[N*N];

		vertices	= new VertexOcean[N_plus_1 * N_plus_1];
		indices		= new u32[N_plus_1 * N_plus_1 * 10];

		// Initialize FFTW.
		//fftwPlan = fftw_plan_dft_c2r_1d(N, hTilde, heightDisplacementOut, FFTW_MEASURE);
		fft = new cFFT(N);

		u32 index = 0;
		vertexCount = 0;

		complex htilde0, htilde0_conjugate;

		for(u32 m_prime = 0; m_prime < N_plus_1; ++m_prime)
		{
			for(u32 n_prime = 0; n_prime < N_plus_1; ++n_prime)
			{
				index = m_prime * N_plus_1 + n_prime;

				htilde0 = HTilde0(m_prime, n_prime);
				
				htilde0_conjugate = HTilde0(-((int)m_prime), -((int)n_prime)).conj();

				// generate vertices.
				vertices[index].htilde0 = glm::vec3(htilde0.a, htilde0.b, 0.0f);
				vertices[index].htilde0conj = glm::vec3(htilde0_conjugate.a, htilde0_conjugate.b, 0.0f);

				float x = (n_prime - N / 2.0f) * length / N;
				float z = (m_prime - N / 2.0f) * length / N;
				
				vertices[index].originalPosition = vertices[index].position = glm::vec3(x, 0.0f, z);
				vertices[index].normal = glm::vec3(0.0f, 1.0f, 0.0f);

				++vertexCount;
			}
		}

		// generate indices.
		indexCount = 0;
		for(u32 m_prime = 0; m_prime < N - 1; ++m_prime)
		{
			for(u32 n_prime = 0; n_prime < N - 1; ++n_prime)
			{
				index = m_prime * N_plus_1 + n_prime;
					//indices[indexCount++] = index;				// lines
					//indices[indexCount++] = index + 1;
					//indices[indexCount++] = index;
					//indices[indexCount++] = index + N_plus_1;
					//indices[indexCount++] = index;
					//indices[indexCount++] = index + N_plus_1 + 1;
					//if (n_prime == N - 1) {
					//	indices[indexCount++] = index + 1;
					//	indices[indexCount++] = index + N_plus_1 + 1;
					//}
					//if (m_prime == N - 1) {
					//	indices[indexCount++] = index + N_plus_1;
					//	indices[indexCount++] = index + N_plus_1 + 1;
					//}
				
				indices[indexCount++] = index;
				indices[indexCount++] = index + N_plus_1;
				indices[indexCount++] = index + 1;

				indices[indexCount++] = index + N_plus_1;
				indices[indexCount++] = index + N_plus_1 + 1;
				indices[indexCount++] = index + 1;

				// first triangle.
				//indices[indexCount++] = index;
				//indices[indexCount++] = index + N_plus_1;
				//indices[indexCount++] = index + N_plus_1 + 1;

				//// second triangle.
				//indices[indexCount++] = index;
				//indices[indexCount++] = index + N_plus_1 + 1;
				//indices[indexCount++] = index + 1;
			}
		}	
		// We need geometry.

		geometry = std::make_shared<Geometry>();
		result &= geometry->Load(o->GetScene().GetGraphicsContext(), vertices, vertexCount, indices, indexCount, vertex_attribs, num_vertex_attribs);

		result &= o->AddComponent("GeometryRenderer");
		GeometryRenderer* geometry_render = o->GetComponent<GeometryRenderer>();
		if(geometry_render == NULL)
			return false;

		geometry_render->SetGeometry(geometry);

		return result;
	}

	void OceanComponent::FixedUpdate( float fixed_delta_time )
	{
	}

	void OceanComponent::Update( float delta_time )
	{
		simulationTime += delta_time * 0.0001f;
		
		//SimulateWavesFFT(simulationTime);
		
		SimulateWavesSimple(simulationTime * 100 * 1000);
		
		geometry->UpdateVertexData(vertices, 0);
	}

	void OceanComponent::Draw( float delta_time )
	{
	}

	// Simulation methods.
	void OceanComponent::ComplexConjugate( complex& c ) const
	{
	}

	void OceanComponent::ComplexMultiplication( complex& result, const complex& a, const complex& b )
	{
	}

	complex OceanComponent::HTilde0( int m_prime, int n_prime )
	{
		std::default_random_engine generator;
		std::normal_distribution<double> distribution(0.0, 1.0);

		float philipps = sqrt(PhillipsSpectrum(m_prime, n_prime) * 0.5f);

		complex r = gaussianRandomVariable();
		return r * philipps;
	}

	float OceanComponent::PhillipsSpectrum( int m_prime, int n_prime )
	{
		// TODO
		glm::vec2 k(A_PI * (2.0f * n_prime - N) / length,
					A_PI * (2.0f * m_prime - N) / length);
		
		float k_length		= k.length();
		const float epsilon = 0.000001;
		if(k_length < epsilon)
			return 0.0f;

		float k_length2 = k_length * k_length;
		float k_length4 = k_length2 * k_length2;

		float k_dot_wind	= glm::normalize(glm::dot(k, wind));
		float k_dot_wind2	= k_dot_wind * k_dot_wind;

		float wind_length2	= glm::dot(wind, wind);
		float L				= wind_length2 / gravity;
		float L2			= L * L;

		// MAYBE TODO: Add damping.
		float damping   = 0.001;
		float l2        = L2 * damping * damping;

		return A * exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_wind2 * exp(-k_length2 * l2);
	}

	complex OceanComponent::HTilde( float time, int m_prime, int n_prime )
	{
		const u32 N_plus_1 = N + 1;
		u32 index = m_prime * N_plus_1 + n_prime;

		complex htilde0(vertices[index].htilde0.x, vertices[index].htilde0.y);

		complex htilde0_conjugate(vertices[index].htilde0conj.x, vertices[index].htilde0conj.y);

		float omega_t = Dispersion(m_prime, n_prime) * time;

		float cos_ = cos(omega_t);
		float sin_ = sin(omega_t);

		complex c0(cos_,  sin_);
		complex c1(cos_, -sin_);

		complex res = htilde0 * c0 + htilde0_conjugate * c1;

		return res;
	}

	float OceanComponent::Dispersion( int m_prime, int n_prime )
	{
		float w_0 = 2.0f * A_PI / 200.0f;
		float kx = A_PI * (2 * n_prime - N) / length;
		float kz = A_PI * (2 * m_prime - N) / length;
		return floor(sqrt(gravity * sqrt(kx * kx + kz * kz)) / w_0) * w_0;
	}

	void OceanComponent::SimulateWavesFFT( float time )
	{
		u32 index = 0;
		float kx, kz, len, lambda = -5.0f;
		for(int m_prime = 0; m_prime < N; ++m_prime)
		{
			kz = A_PI * (2.0f * m_prime - N) / length;
			for(int n_prime = 0; n_prime < N; ++n_prime)
			{
				kx = A_PI*(2 * n_prime - N) / length;
				len = sqrt(kx * kx + kz * kz);

				index = m_prime * N + n_prime;

				hTilde[index] = HTilde(time, m_prime, n_prime);

				if (len < 0.000001f) 
				{
					hTildeX[index]     = complex(0.0f, 0.0f);
					hTildeZ[index]     = complex(0.0f, 0.0f);
				} 
				else 
				{
					hTildeX[index]     = hTilde[index] * complex(0, -kx/len);
					hTildeZ[index]     = hTilde[index] * complex(0, -kz/len);
				}
			}
		}

		//fftw_execute(fftwPlan);

		for (int m_prime = 0; m_prime < N; m_prime++) 
		{
			fft->fft(hTilde, hTilde, 1, m_prime * N);
			fft->fft(hTildeX, hTildeX, 1, m_prime * N);
			fft->fft(hTildeZ, hTildeZ, 1, m_prime * N);
		}
		for (int n_prime = 0; n_prime < N; n_prime++) 
		{
			fft->fft(hTilde, hTilde, N, n_prime);
			fft->fft(hTildeX, hTildeX, 1, n_prime);
			fft->fft(hTildeZ, hTildeZ, 1, n_prime);
		}

		index = 0;
		u32 index1 = 0;
		const u32 N_plus_1 = N + 1;

		int sign;
		float signs[] = { 1.0f, -1.0f };

		for (int m_prime = 0; m_prime < N; m_prime++) 
		{
			for (int n_prime = 0; n_prime < N; n_prime++) 
			{
				index  = m_prime * N + n_prime;		// index into h_tilde..
				index1 = m_prime * N_plus_1 + n_prime;	// index into vertices

				sign = signs[(n_prime + m_prime) & 1];

				hTilde[index]  = hTilde[index] * sign;

				// height.
				vertices[index1].position.y = hTilde[index].a;

				// displacement_img.
				hTildeX[index]  = hTildeX[index] * sign;
				hTildeZ[index]  = hTildeZ[index] * sign;
				vertices[index1].position.x = vertices[index1].originalPosition.x + hTildeX[index].a * lambda;
				vertices[index1].position.z = vertices[index1].originalPosition.z + hTildeZ[index].a * lambda;

				// tiling.
				if (n_prime == 0 && m_prime == 0) 
				{
					vertices[index1 + N + N_plus_1 * N].position.y = hTilde[index].a;
				}
				if (n_prime == 0) 
				{
					vertices[index1 + N].position.y = hTilde[index].a;
				}
				if (n_prime == 0 && m_prime == 0) 
				{
					vertices[index1 + N_plus_1 * N].position.y = hTilde[index].a;
				}


			}
		}
	}

	void OceanComponent::SimulateWavesSimple( float time )
	{
		u32 index = 0;

		const float wavelength[] = {0.05f, 0.1f, 0.1f, 0.07f};
		const float velocity[] = {0.7f, 0.3f, 0.1f, 0.4f};
		const float amplitude[] = {0.4f, 2.0f, 0.5f, 0.1f};
		const float steepnes[] = {0.3f, 1.0f, 0.7f, 0.1f};

		const float frequency[] = {2 * A_PI * wavelength[0], 2 * A_PI * wavelength[1], 2 * A_PI * wavelength[2], 2 * A_PI * wavelength[3]};

		const float theta[] = {velocity[0] * frequency[0], velocity[1] * frequency[1], velocity[2] * frequency[2], velocity[3] * frequency[3]};

		float theta_t[] = {theta[0] * time, theta[1] * time, theta[2] * time, theta[3] * time};

		glm::vec3 directions[] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(6.5f, 0.0f, -2.5f), glm::vec3(15.0f, 0.0f, 5.0f)};

		glm::vec3 w = glm::vec3(wind.x, 0.0f, wind.y);

		for (int m_prime = 0; m_prime < N; m_prime++) 
		{
			for (int n_prime = 0; n_prime < N; n_prime++) 
			{
				index  = m_prime * N + n_prime;

				float height = 0.0f;
				for(int i = 0; i < 4; ++i)
				{
					float pos_dot_dir = glm::dot(vertices[index].position, directions[i]);
					float S = pos_dot_dir * frequency[i] + theta_t[i];

					height += amplitude[i] * pow((sin(S) + 1) * 0.5f, steepnes[i]);
				}
				
				vertices[index].position.y = height;

			}
		}
	}

}
*/
