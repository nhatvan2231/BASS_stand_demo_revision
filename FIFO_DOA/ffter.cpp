#include "ffter.h"

bool FFTer::Initialize() {
	if(input == NULL) return false;

	Q_init = true;
	plan = fftw_plan_many_dft_r2c(1, &this->N_fft, N_chan,
				input, NULL,
				N_chan, 1,
				(fftw_complex*) output, NULL,
				N_chan, 1,
				FFTW_ESTIMATE);
	return true;
}
bool FFTer::FFT() {
	if(input == NULL || !Q_init) return false;

	buffer_lock.lock();
	fftw_execute(plan);
	buffer_lock.unlock();
	return true;
}
