#pragma once
#include "Component.h"
#include "Types.h"
#include "Geometry.h"

#include <complex>

#include <fftw3.h>
#include <blitz/array.h>
#include <glm/glm.hpp>
#include <ImathRandom.h>

#include <memory>

namespace acqua
{
	// Type definitions.
	typedef float							Real;
	typedef std::complex<Real>				ComplexReal;
	typedef blitz::Array<Real, 1>			VectorReal;
	typedef blitz::Array<Real, 2>			MatrixReal;
	typedef blitz::Array<glm::vec3, 2>		Vector3Array;
	typedef blitz::Array<ComplexReal, 2>	MatrixComplex;

	// useful constants.
	const float k_Gravity = 9.81f;
	const double k_Pi = 3.14159265358979323846264338327950288;

	const ComplexReal k_Minus_i(0,-1);
	const ComplexReal k_Plus_i(0,1);

	// Ocean Settings
	struct OceanSettings
	{
		Real V;
		Real l;
		Real A;
		Real W;
		Real windAlignment;

		Real dampReflections;
		Real depth;

		Real chopAmount;

		Real foamSlopeRatio; //Decides the slope ratio to start the foam;
		Real foamFader;	//Decides how much the foam increases and decreases over frames.

		OceanSettings() : V(4.0f), l(2.0f), A(1.0f), W(2.0f), windAlignment(2.0f), dampReflections(0.5f), depth(200.0f), chopAmount(0.5f), foamSlopeRatio(0.07f), foamFader(0.1f)
		{

		}
	};

	// Simulate an ocean using Tessendorf's algorithm and FFT.
	class OceanComponent : public Component
	{
	private:
		//typedef fftw_complex complex;

		struct VertexOcean
		{
			glm::vec3	position; 
			glm::vec3	normal;
			glm::vec3	originalPosition;
			glm::vec2	texcoords;
			Real		foamAmount;
		};

	public:
		OceanComponent(void);
		~OceanComponent(void);

		virtual bool Init( GameObject* o );

		virtual void FixedUpdate( float fixed_delta_time );

		virtual void Update( float delta_time );

		virtual void Draw( float delta_time );

		// Public interface
		void ResetOcean(const OceanSettings& settings);

	private:
		// Ocean simulation methods.
		Real Ph(Real k_x, Real k_z) const; // Phillips spectrum.
		
		Real Wavelength(Real k_) const
		{
			return 2.0f * k_Pi / k_;
		}

		Real Omega(Real k_) const
		{
			return sqrt(k_Gravity * k_ * tanh(k_ * depth) );
		}

		void SimulateOceanFFT(float t, float scale);

	private:
		
		/*
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
		*/
		
		// Geometry generation.
		VertexOcean*	vertices;
		u32				vertexCount;
		u32*			indices;
		u32				indexCount;

		// Ocean simulation members.
		
		Real simulationTime;
		int seed; // Random seed.

		// Dimensions of the grid.
		u32 M;
		u32 N;

		// Spatial size of the grid.
		Real LX;
		Real LZ;

		// Simulation.
		Real V;	// Speed of the waves.
		Real L; // Largest wave length at velocity V.
		Real l; // Shortest wave length. Used for pruning out very small waves.
		Real A; // Amplitude (approximate wave height). 
		
		// wind.
		Real W; // Wind direction in radians.
		Real WX; Real WZ; // Wind directions.
		Real windAlignment; // How close waves travel in the direction of wind.
		
		Real dampReflections; // Damps out the negative direction waves. 
		Real depth; // Depth of the ocean.

		Real chopAmount; // Amount of chop displacement that is applied to the input points.
		
		// Rendering.
		Real foamSlopeRatio; //Decides the slope ratio to start the foam;
		Real foamFader;	//Decides how much the foam increases and decreases over frames.

		// Direction vectors per grid point.
		VectorReal kx; VectorReal kz;
		MatrixReal k; // Matrix of their magnitudes.

		MatrixComplex h0;
		MatrixComplex h0Minus;

		// FFT related members.
		MatrixComplex FFTIn; // Input to the plans.
		MatrixComplex hTilda;

		fftwf_plan displacementYPlan;
		MatrixReal displacementY; // Output for the above plan.

		fftwf_plan displacementXPlan;
		MatrixReal displacementX;

		fftwf_plan displacementZPlan;
		MatrixReal displacementZ;

		fftwf_plan normalXPlan;
		MatrixReal normalX;

		fftwf_plan normalZPlan;
		MatrixReal normalZ;

		Vector3Array normalArray;

		// Engine related members.
		std::shared_ptr<Geometry> geometry;
	};

	REGISTER_COMPONENT(OceanComponent)
}

