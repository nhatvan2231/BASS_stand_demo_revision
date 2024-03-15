#ifndef ANGLER_H
#define ANGLER_H

#include <thread>

#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_complex.h>

using namespace std;

class Angler
{
	public:
		// Constructor:
		// Given the high and low frequency, it will monitor a bandwidth of bins with a scaled bin width
		Angler(unsigned int N_chan, unsigned int N_dim, unsigned int N_bins, double angle_res):
			N_chan(N_chan),
			N_dim(N_dim),
			N_bins(N_bins),
			angle_res(angle_res),
			N_phi(ceil(360.0/angle_res)),
			N_theta(ceil(180.0/angle_res))
			{
				ones = gsl_vector_complex_alloc(N_theta*N_phi);
				gsl_vector_complex_set_all(ones,{1.0,0.0});

				y = new gsl_vector_complex*[N_bins];
				Y = new gsl_matrix_complex*[N_bins];
				A = new gsl_matrix_complex*[N_bins];
				Ah = new gsl_matrix_complex*[N_bins];

				quiet_bin = new bool[N_bins];
				err = new double[N_bins];

				angles = new double*[N_bins];


				temp_vec_view = new gsl_vector_complex_view[N_bins];
				temp_err = new double[N_bins];

				temp_vec = new gsl_vector_complex*[N_bins];
				temp_mat = new gsl_matrix_complex*[N_bins];

				for(int b = 0; b < N_bins; ++b)
					quiet_bin[b] = false;
				/*
				for(int b = 0; b < N_bins; ++b) {
					y[b] = gsl_vector_complex_calloc(N_chan);
					Y[b] = gsl_matrix_complex_calloc(N_chan,N_theta*N_phi);
					A[b] = gsl_matrix_complex_calloc(N_chan,N_theta*N_phi);
					Ah[b] = gsl_matrix_complex_calloc(N_theta*N_phi,N_chan);

					quiet_bin[b] = false;

					angles[b] = new double[2];

					temp_vec[b] = gsl_vector_complex_alloc(N_phi*N_theta);
					temp_mat[b] = gsl_matrix_complex_alloc(N_chan,N_phi*N_theta);
				}
				*/

			}

		~Angler() {
			gsl_vector_complex_free(ones);

			for(int b = 0; b < N_bins; ++b) {
				if(quiet_bin[b]) {
					delete[] angles[b];

					gsl_vector_complex_free(y[b]);
					gsl_matrix_complex_free(Y[b]);

					gsl_matrix_complex_free(A[b]);
					gsl_matrix_complex_free(Ah[b]);

					gsl_vector_complex_free(temp_vec[b]);
					gsl_matrix_complex_free(temp_mat[b]);
				}
			}

			delete[] y;
			delete[] Y;

			delete[] A;
			delete[] Ah;
			delete[] angles;

			delete[] temp_vec;
			delete[] temp_mat;
		}

		// Initializes R and A
		bool Initialize(const char* array_geometry, const char* quiet_freqs, double bin_width, double speed);

		// Takes data from FFTer
		void ProcessDataAll(gsl_complex* present);	// Multithreaded

		void FindAnglesAll();	// Multithreaded

		bool* quiet_bin;
		double* err;

		double** angles;					// Angles for each bin
	private:
		void ProcessData(unsigned int chan, gsl_complex* present);
		void FindAngles(unsigned int bin);

		unsigned int N_chan;				// Number of channels
		unsigned int N_dim;				// Number of Spacial dimensions
		unsigned int N_bins;				// Number of bins (frequencies) to process

		double angle_res;					// Angular resolution
		unsigned int N_phi;				// Number of azimuthal angles
		unsigned int N_theta;			// Number of polar angles

		gsl_vector_complex** y;			// Vector of present data
		gsl_matrix_complex** Y;			// Matrix with present data as columns
		gsl_matrix_complex** A;			// "Steering" vectors
		gsl_matrix_complex** Ah;		// ConjTrans/N_chan of "Steering" vectors

		gsl_vector_complex* ones;

		gsl_vector_complex** temp_vec;
		gsl_matrix_complex** temp_mat;
		gsl_vector_complex_view* temp_vec_view;
		double* temp_err;
};
#endif
