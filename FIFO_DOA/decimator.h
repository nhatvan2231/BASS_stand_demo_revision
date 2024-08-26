#ifndef DECIMATOR_H
#define DECIMATOR_H

#include <gsl/gsl_complex.h>
#include <thread>
#include <mutex>

#include "Iir.h"

#define ORDER 5

template <class T>
class Decimator {
	public:
		Decimator(unsigned int N_chan, double sampling_freq, double cutoff_freq, double stopband_ripple, unsigned int d_factor, unsigned int N_isamples):
			N_chan(N_chan),
			d_factor(d_factor),
			N_isamples(N_isamples)
			{
				buffer = new double[N_chan*N_isamples/d_factor];
				memset(buffer, 0.0f, sizeof(buffer));

				low_pass = new Iir::ChebyshevII::LowPass<ORDER>[N_chan];

				for(int c = 0; c < N_chan; ++c) {
					low_pass[c].setup(
					sampling_freq, cutoff_freq,
					stopband_ripple);
				}
			}

		~Decimator()
		{
			delete[] low_pass;
			delete[] buffer;
		}

		double* buffer;
		std::mutex buffer_lock;

		void ReadAll(T* data);
	private:
		void Read(T* data, unsigned int chan);
		Iir::ChebyshevII::LowPass<ORDER>* low_pass;
		unsigned int d_factor;

		unsigned int N_chan;

		unsigned int N_isamples;
};

#undef ORDER
#endif
