#pragma once
#include "Component.h"
#include "Types.h"
#include "Geometry.h"

#include "fft.h"

#include <fftw3.h>
#include <glm/glm.hpp>
#include <memory>

namespace acqua
{
	// Simulate an ocean using Tessendorf's algorithm and FFT.
	class OceanComponent : public Component
	{
	private:
		//typedef fftw_complex complex;

		struct VertexOcean
		{
			glm::vec3 position; 
			glm::vec3 normal;
			glm::vec3 htilde0;
			glm::vec3 htilde0conj; // conjugate
			glm::vec3 originalPosition;
		};

	public:
		OceanComponent(void);
		~OceanComponent(void);

		virtual bool Init( GameObject* o );

		virtual void FixedUpdate( float fixed_delta_time );

		virtual void Update( float delta_time );

		virtual void Draw( float delta_time );

	private:
		// FFTW simple extensions.
		void ComplexConjugate(complex& c) const;
		void ComplexMultiplication(complex& result, const complex& a, const complex& b);

		// Ocean simulation methods.
		complex HTilde0(int m_prime, int n_prime);
		float PhillipsSpectrum(int m_prime, int n_prime);

		complex HTilde(float time, int m_prime, int n_prime);
		float Dispersion(int m_prime, int n_prime);

		void SimulateWavesFFT(float time);

	private:
		float simulationTime;

		// Geometry generation.
		VertexOcean*	vertices;
		u32				vertexCount;
		u32*			indices;
		u32				indexCount;

		// Ocean simulation parameters.
		float		gravity; // Gravity constant. Usually 9.8f.
		u32			N; // Dimension. Should be a power of 2.
		float		A; // Phillips spectrum parameter. Height of waves. 
		float		length;
		glm::vec2	wind; // Wind direction.

		// FFT variables.
		cFFT*		fft;
		fftw_plan	fftwPlan;
		complex*	hTilde;
		complex*	hTildeX;
		complex*	hTildeZ;

		std::shared_ptr<Geometry> geometry;
	};

	REGISTER_COMPONENT(OceanComponent)
}

