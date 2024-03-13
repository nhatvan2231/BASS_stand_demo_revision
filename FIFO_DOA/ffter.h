#ifndef FFTER_H
#define FFTER_H

#include <fftw3.h>
#include <gsl/gsl_complex.h>

#include <thread>
#include <mutex>

class FFTer {
	public:
		FFTer(unsigned int N_chan, unsigned int N_fft):
			N_chan(N_chan),
			N_fft(N_fft),
			N_bins(N_fft/2 + N_chan),
			Q_init(false)
			{
				input = NULL;
				output = new gsl_complex[N_chan*N_bins];

				for(int cb = 0; cb < N_chan*N_bins; ++cb)
					output[cb] = {0.0,0.0};
			}
		~FFTer() {
			if(Q_init)
				fftw_destroy_plan(plan);
			delete[] output;
		}

		double* input;
		gsl_complex* output;

		std::mutex buffer_lock;

		bool Initialize();
		bool FFT();
	private:
		bool Q_init;
		fftw_plan plan;

		const int N_chan;
		const int N_fft;
		const int N_bins;
};

#undef ORDER
#endif
