#include "OceanComponent.h"

#include "GameObject.h"
#include "GraphicsContext.h"
#include "Math.h"

#include <random>
#include <cmath>

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
		gravity = 9.8f;
		N = 64;
		length = 64.0f;
		wind = glm::vec2(32.0f,32.0f);
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
		for(u32 m_prime = 0; m_prime < N; ++m_prime)
		{
			for(u32 n_prime = 0; n_prime < N; ++n_prime)
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
				// first triangle.
				indices[indexCount++] = index;
				indices[indexCount++] = index + N_plus_1;
				indices[indexCount++] = index + N_plus_1 + 1;

				// second triangle.
				indices[indexCount++] = index;
				indices[indexCount++] = index + N_plus_1 + 1;
				indices[indexCount++] = index + 1;
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
		SimulateWavesFFT(simulationTime);

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
		float kx, kz, len, lambda = -1.0f;
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

				// displacement.
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

}
