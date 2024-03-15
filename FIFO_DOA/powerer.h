#ifndef POWERER_H
#define POWERER_H

#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_complex.h>
#include <thread>
using namespace std;

class Powerer
{
	public:
		// Constructor:
		// Given the high and low frequency, it will monitor a bandwidth of bins with a scaled bin width
		Powerer(unsigned int N_chan, unsigned int N_bins):
			N_chan(N_chan),
			N_bins(N_bins)
		{
			y = new gsl_vector_complex*[N_bins];

			quiet_bin = new bool[N_bins];

			power_bin = new bool[N_bins];
			power = new double[N_bins];

			for(int b = 0; b < N_bins; ++b)
			{
				y[b] = gsl_vector_complex_calloc(N_chan);

				quiet_bin[b] = false;
				power_bin[b] = true;
			}

		}

		~Powerer()
		{
			for(int b = 0; b < N_bins; ++b)
				gsl_vector_complex_free(y[b]);

			delete[] y;

			delete[] quiet_bin;

			delete[] power_bin;
			delete[] power;
		}

		// Initializes Quiet Freqs
		bool Initialize(const char* quiet_freqs, double bin_width);

		// Takes data from FFTer
		void ProcessDataAll(gsl_complex* present);	// Multithreaded

		void PowerCalc();

		bool* quiet_bin;

		bool* power_bin;
		double* power;
	private:
		void ProcessData(unsigned int chan, gsl_complex* present);
		unsigned int N_chan;				// Number of channels
		unsigned int N_bins;				// Number of bins to process

		gsl_vector_complex** y;			// Vector of present data
};
#endif
