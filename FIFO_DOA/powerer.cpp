#include "powerer.h"

bool Powerer::Initialize(const char* quiet_freqs, double bin_width)
{
	FILE* f_freqs = fopen(quiet_freqs,"r");

	double tmp_double = 0;

	if(f_freqs != NULL)
	{
		while(fscanf(f_freqs, "%lf", &tmp_double) == 1)
		{
			quiet_bin[int(ceil(tmp_double/bin_width))] = true;
			quiet_bin[int(tmp_double/bin_width)] = true;
		}

		fclose(f_freqs);

		return true;
	}
	else
	{
		fclose(f_freqs);
		return false;
	}
}

void Powerer::ProcessData(unsigned int chan, gsl_complex* present)
{
	for(int b = 0; b < N_bins; ++b)
	{
		if(quiet_bin[b])
			gsl_vector_complex_set(
					y[b], chan,
					present[b*N_chan + chan]
				);
	}
}
void Powerer::ProcessDataAll(gsl_complex* present)
{
	thread Thread[N_chan];
	for(int c = 0; c < N_chan; ++c)
		Thread[c] = thread(&Powerer::ProcessData, this, c, present);

	for(int c = 0; c < N_chan; ++c)
		Thread[c].join();
}

void Powerer::PowerCalc()
{
	for(int b = 0; b < N_bins; ++b)
		if(quiet_bin[b])
			power[b] = gsl_blas_dznrm2(y[b]);
}
