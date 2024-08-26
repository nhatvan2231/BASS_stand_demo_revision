#include "decimator.h"

template <class T>
void Decimator<T>::Read(T* data, unsigned int chan) {
	unsigned int head = 0;
	unsigned int d_state = 0;

	for(int s = 0; s < N_isamples; ++s) {
		if(d_state == d_factor) {
			buffer_lock.lock();
			buffer[head*N_chan + chan] = low_pass[chan].filter(data[s*N_chan + chan]);
			buffer_lock.unlock();

			++head;
			d_state = 0;
		} else {
			buffer_lock.lock();
			low_pass[chan].filter(data[s*N_chan + chan]);
			buffer_lock.unlock();
		}

		++d_state;
	}
}

template <class T>
void Decimator<T>::ReadAll(T* data) {
	std::thread Thread[N_chan];
	for(int c = 0; c < N_chan; ++c)
		Thread[c] = std::thread(&Decimator<T>::Read, this, data, c);

	for(int c = 0; c < N_chan; ++c)
		Thread[c].join();
}

template class Decimator <float>;
template class Decimator <double>;
