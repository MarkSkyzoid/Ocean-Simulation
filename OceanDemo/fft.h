#ifndef FFT_H
#define FFT_H

#include <math.h>
#include "my_complex.h"
namespace acqua
{
	class cFFT 
	{
	  private:
		unsigned int N, which;
		unsigned int log_2_N;
		float pi2;
		unsigned int *reversed;
		complex **W;
		complex *c[2];
	  protected:
	  public:
		cFFT(unsigned int N);
		~cFFT();
		unsigned int reverse(unsigned int i);
		complex w(unsigned int x, unsigned int N);
		void fft(complex* input, complex* output, int stride, int offset);
	};
}
#endif