#include "angler.h"

bool Angler::Initialize(const char* array_geometry, const char* quiet_freqs, double bin_width, double speed) {
	FILE* f_array = fopen(array_geometry,"r");
	FILE* f_freqs = fopen(quiet_freqs,"r");

	double tmp_double = 0;
	gsl_matrix* R = gsl_matrix_alloc(N_chan,N_dim);	// Mic positions

	if(f_array != NULL and f_freqs != NULL) {
		for(int c = 0; c < N_chan; ++c) {
			for(int d = 0; d < N_dim; ++d) {
				fscanf(f_array, "%lf", &tmp_double);
				gsl_matrix_set(
						R, c, d,
						tmp_double
					);
			}
		}
		while(fscanf(f_freqs, "%lf", &tmp_double) == 1) {
			quiet_bin[int(ceil(tmp_double/bin_width))] = true;
			quiet_bin[int(tmp_double/bin_width)] = true;
		}
		for(int b = 0; b < N_bins; ++b) {
			if(quiet_bin[b]) {
				y[b] = gsl_vector_complex_calloc(N_chan);
				Y[b] = gsl_matrix_complex_calloc(N_chan,N_theta*N_phi);
				A[b] = gsl_matrix_complex_calloc(N_chan,N_theta*N_phi);
				Ah[b] = gsl_matrix_complex_calloc(N_theta*N_phi,N_chan);
				angles[b] = new double[2];
				temp_vec[b] = gsl_vector_complex_alloc(N_phi*N_theta);
				temp_mat[b] = gsl_matrix_complex_alloc(N_chan,N_phi*N_theta);
			}
		}

		gsl_vector* slowness = gsl_vector_alloc(N_dim);	// Slowness vector
		gsl_vector* T = gsl_vector_alloc(N_chan);	// Time differences

		for(int p = 0; p < N_phi; ++p)
		for(int t = 0; t < N_theta; ++t) {
			gsl_vector_set(
					slowness, 0,
					cos(p*angle_res*M_PI/180)*sin(t*angle_res*M_PI/180)/speed
					);
			gsl_vector_set(
					slowness, 1,
					sin(p*angle_res*M_PI/180)*sin(t*angle_res*M_PI/180)/speed
					);
			gsl_vector_set(
					slowness, 2,
					cos(t*angle_res*M_PI/180)/speed
					);
			gsl_blas_dgemv(CblasNoTrans, 1.0, R, slowness, 0, T);
			for(int b = 0; b < N_bins; ++b) {
				if(quiet_bin[b]) {
					for(int c = 0; c < N_chan; ++c) {
						gsl_matrix_complex_set(
								A[b], c, p*N_theta + t,
								gsl_complex_polar(1.0,2*M_PI*b*bin_width*gsl_vector_get(T,c))
							);
					}
				}
			}
		}
		for(int b = 0; b < N_bins; ++b) {
			if(quiet_bin[b]) {
				gsl_matrix_complex_conjtrans_memcpy(Ah[b],A[b]);
				gsl_matrix_complex_scale(Ah[b],{1.0/N_chan,0.0});
			}
		}


		gsl_matrix_free(R);
		gsl_vector_free(slowness);
		gsl_vector_free(T);
		fclose(f_array);
		fclose(f_freqs);

		return true;
	}
	else
	{
		fclose(f_array);
		fclose(f_freqs);
		return false;
	}
}

void Angler::ProcessData(unsigned int chan, gsl_complex* present)
{
	for(int b = 0; b < N_bins; ++b) {
		if(quiet_bin[b])
			gsl_vector_complex_set(
					y[b], chan,
					present[b*N_chan + chan]
				);
	}
}
void Angler::ProcessDataAll(gsl_complex* present)
{
	thread Thread[N_chan];
	for(int c = 0; c < N_chan; ++c)
		Thread[c] = thread(&Angler::ProcessData, this, c, present);

	for(int c = 0; c < N_chan; ++c)
		Thread[c].join();
}

void Angler::FindAngles(unsigned int bin)
{
	if(quiet_bin[bin]) {
		err[bin]= INT_MAX;

		gsl_matrix_complex_set_zero(Y[bin]);
		gsl_matrix_complex_memcpy(temp_mat[bin], A[bin]);

		// Makes matrix of y[bin] as columns
		gsl_blas_zgeru({1.0,0.0}, y[bin], ones, Y[bin]
			);

		// A^H*y/N_chan
		gsl_blas_zgemv(CblasNoTrans,
				{1.0,0.0}, Ah[bin], y[bin], {0.0,0.0}, temp_vec[bin]
			);

		// A*(A^H*y/N_chan)
		gsl_matrix_complex_scale_columns(temp_mat[bin],temp_vec[bin]);

		// Y - A*(A^H*y/N_chan)
		gsl_matrix_complex_sub(Y[bin],temp_mat[bin]);
		for(int i = 0; i < N_phi*N_theta; ++i) {
			temp_vec_view[bin] = gsl_matrix_complex_column(Y[bin],i);
			temp_err[bin] = gsl_blas_dznrm2(&temp_vec_view[bin].vector)/gsl_blas_dznrm2(y[bin]);

			if(err[bin] > temp_err[bin]) {
				err[bin] = temp_err[bin];
				angles[bin][0] = i/N_theta * angle_res;
				angles[bin][1] = i%N_theta * angle_res;
			}
		}
	}
}

void Angler::FindAnglesAll()
{
	thread Thread[N_bins];
	for(int b = 0; b < N_bins; ++b)
		if(quiet_bin[b])
			Thread[b] = thread(&Angler::FindAngles, this, b);

	for(int b = 0; b < N_bins; ++b)
		if(quiet_bin[b])
			Thread[b].join();
}
