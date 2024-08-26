#ifndef STATER_H
#define STATER_H

#include <thread>

#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_complex.h>

using namespace std;

class Stater
{
	public:
		// Constructor:
		// Given the high and low frequency, it will monitor a bandwidth of bins with a scaled bin width
		Stater(unsigned int N_vars, unsigned int N_moms, double learn):
			N_vars(N_vars),
			N_moms(N_moms),
			learn(learn),
			stubb(1-learn)
		{
			moms = new double*[N_vars];
			mean = new double[N_vars];
			variance = new double[N_vars];
			for(int v = 0; v < N_vars; ++v)
			{
				moms[v] = new double[N_moms];
				for(int m = 0; m < N_moms; ++m)
				{
					if(m == 1)
						moms[v][m] = 1.0;
					else
						moms[v][m] = 0.0;
				}
			}
		}

		~Stater()
		{
			for(int v = 0; v < N_vars; ++v)
				delete[] moms[v];
			delete[] moms;
		}

		bool UpdateMoms(double* data, unsigned int var);
		bool UpdateMomsAll(double* data);

		double** moms;
		double* mean;
		double* variance;

		unsigned int N_moms;
		unsigned int N_vars;
	private:
		unsigned int N_stats;

		double learn;
		double stubb;
		double* stats;
};
#endif
