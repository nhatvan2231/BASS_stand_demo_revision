#include <thread>
#include <atomic>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <float.h>
#include <mutex>
#include <math.h>

#include "ffter.h"
#include "powerer.h"
#include "angler.h"
#include "decimator.h"
#include "stater.h"
#include "sighandler.h"
#include "zdaq.h"
#include "ravg.h"

// TODO: include messenger class
#include "messenger.h"

#define N_CHAN 19
#define N_FFT 4096
#define ZDAQ_SAMPLES 20480

using namespace std;
using namespace std::chrono;

const char array_geometry[]="/home/ncpa/FIFO_DOA/array_geometry.dat";
const char quiet_freqs[]="/home/ncpa/FIFO_DOA/quiet_freqs/0040_quiet_freqs.dat";

const double sampling_freq = 48000.0;		// Hz
const double lo_freq = 100.0;					// Hz
const double hi_freq = 2000.0;				// Hz
const unsigned int N_bins = N_FFT/2 + 1;

const unsigned int d_factor = 10;
const double bin_width = sampling_freq/(d_factor*N_FFT);

const double angle_res = 5;	// Degrees
const double speed = 343.0;	// meters/second

const double stopband_ripple = 20;
const double cutoff_freq = 1500.0;

// Trigger thresholds (for detector_thread)
double err_thresh = 0.7;
//double rel_power_thresh = 0.0, abs_power_thresh = 0.4;
double rel_power_thresh = 0.35, abs_power_thresh = 0.2;
int p_trig_thresh = 10;


// Historical buffers, etc
mutex backtrack_lock;
//float history[N_CHAN][N_FFT*d_factor];
//float current[N_CHAN][N_FFT*d_factor];

// Number of (n_samples * n_channels) in the buffer
#define META_BUFF 10
size_t algo_buff_elements = N_CHAN*META_BUFF*ZDAQ_SAMPLES;
float* algo_buffer;
float* algo_head;

float* current;
float* history;

float* tmp_backtrack_buffer = NULL;


size_t buffer_size = N_FFT*sizeof(double);

FFTer fft(N_CHAN, N_FFT);
Stater stats(N_bins, 2, 0.05);


// TODO: Messenger object pointer & timestamp string
Messenger* m;
string* detect_ts;

// Zdaq object pointer
Zdaq* daq;

// function prototypes
bool find_peak();
bool find_direction(float*);


void save_history(float* buff)
{
	algo_head += N_CHAN*ZDAQ_SAMPLES;

	//if( current > (algo_buffer + algo_buff_elements - N_CHAN*N_FFT*d_factor) )
	if( algo_head < (algo_buffer + algo_buff_elements) ) {
		current = algo_head - N_CHAN*(N_FFT*d_factor - ZDAQ_SAMPLES);
		history = current - N_CHAN*N_FFT*d_factor;
	} else {
		current = algo_buffer + N_CHAN*(N_FFT*d_factor - ZDAQ_SAMPLES);
		history = algo_buffer + algo_buff_elements - N_CHAN*N_FFT*d_factor;

		// TODO: If this is too slow, put this after the memcpy of buff
		memcpy(algo_buffer, algo_buffer + algo_buff_elements - N_CHAN*N_FFT*d_factor, sizeof(float)*N_CHAN*N_FFT*d_factor);
		algo_head = algo_buffer + N_CHAN*N_FFT*d_factor;
	}

	backtrack_lock.lock();
	memcpy(algo_head, buff, sizeof(float)*N_CHAN*ZDAQ_SAMPLES);
	backtrack_lock.unlock();

	/*
	history = current;
	current += N_CHAN*ZDAQ_SAMPLES;

	// Reset current if it oversteps the buffer
	if( current >= (algo_buffer + algo_buff_elements) )
		// Copy last set of overall buffer to beginning
		memcpy(algo_buffer, current-N_CHAN*ZDAQ_SAMPLES, sizeof(float)*N_CHAN*ZDAQ_SAMPLES);
		current = algo_buffer + N_CHAN*ZDAQ_SAMPLES;

	backtrack_lock.lock();
	memcpy(current, buff, sizeof(float)*N_CHAN*ZDAQ_SAMPLES);
	//memcpy(&history[0][0], &current[0][0], sizeof(float) * N_CHAN * N_FFT * d_factor);
	//memcpy(&current[0][0], buff, sizeof(float) * N_CHAN * N_FFT * d_factor);
	backtrack_lock.unlock();
	*/
}


void detector_thread()
{
	// zero out historical buffers
	memset(algo_buffer, 0.0f, sizeof(algo_buffer));
	//memset(history, 0, sizeof(float) * N_CHAN * N_FFT * d_factor);
	//memset(current, 0, sizeof(float) * N_CHAN * N_FFT * d_factor);

	// Decimator & FFTer init
	Decimator<float> decimator(N_CHAN, sampling_freq, cutoff_freq, stopband_ripple, d_factor, N_FFT*d_factor);
	fft.input = decimator.buffer;
	if (!fft.Initialize()) {
		fprintf(stderr, "Failed to initialize FFTer\n"); fflush(stderr);
		exit(1);
	}

	// Powerer init
	Powerer pwer(N_CHAN, N_bins);
	if(!pwer.Initialize(quiet_freqs, bin_width))
	{
		fprintf(stderr,"Failed to initialize Powerer\n"); fflush(stderr);
		exit(1);
	}

	// Statistical information
	double tmp;

	unsigned int p_counter = 0, waiter = 0;
	unsigned int wait_period = 1*N_FFT*d_factor/ZDAQ_SAMPLES;	// Number of FFTs to wait before starting detecter

	printf("Thresholds\nPower Trigger: %d\nRelative Power: %f\nAbsolute Power: %f\n", p_trig_thresh, rel_power_thresh, abs_power_thresh);

	// read status struct
	Zdaq::zdaq_read_t read_status;

	daq->start();
	while(!shutdown_signal and !daq->ZDAQ_IS_DONE)
	{
		if(daq->ZDAQ_IS_READ)
		{
			// TODO: locking this entire block is only for running pcm files at highest possible speed
			//daq->ZDAQ_LOCK.lock();

			// check the read status
			read_status = daq->read();
			// Send time stamp of current fft
			//m->send_time();

			// Read data from the input device
			daq->ZDAQ_LOCK.lock();
			save_history(read_status.buff_ptr);
			daq->ZDAQ_LOCK.unlock();

			// decimate the data
			decimator.ReadAll(current);

			// fft the decimated data
			fft.FFT();

			daq->ZDAQ_IS_READ = false;

			pwer.ProcessDataAll(fft.output);
			pwer.PowerCalc();

			p_counter = 0;
			if(waiter < wait_period)
			{
				stats.UpdateMomsAll(pwer.power);
				++waiter;
			}
			else
			{
				for(int b = 0; b < N_bins; ++b)
				{
					if(pwer.quiet_bin[b])
					{
						tmp = pwer.power[b] - stats.mean[b];
						if(tmp*tmp > rel_power_thresh*stats.variance[b] and pwer.power[b] > abs_power_thresh)
						{
							pwer.power_bin[b] = true;
							++p_counter;
						}
						else
						{
							pwer.power_bin[b] = false;
							stats.UpdateMoms(pwer.power, b);
						}
					}
				}
			}

			//if(p_counter > p_trig_thresh and waiter > wait_period)
			if(p_counter > p_trig_thresh and waiter > wait_period)
			{
				waiter = 0;
				printf("Detected!\n");

				// TODO: send the detect message
				string ts = m->send_detect();
				detect_ts = &ts;

				find_peak();

				// write file if failed to find peak or angle
				/*
				if (!find_peak()) {
					// save the history & current buffers to a file
					char filename[100];
					char tmp_ts[80];

					timeval current_time;
					gettimeofday(&current_time, NULL);
					strftime(tmp_ts, 80, "%y-%m-%d_%H_%M_%S", localtime(&current_time.tv_sec));
					sprintf(filename, "debug_pcm/%s.pcm", tmp_ts);

					FILE* fp;
					fp = fopen(filename, "wb");
					fwrite(history, sizeof(float), N_CHAN * N_FFT * d_factor, fp);
					fwrite(current, sizeof(float), N_CHAN * N_FFT * d_factor, fp);
					fclose(fp);

					printf("Wrote debug file '%s'\n", filename);
				}
				*/
			}
			else
				++waiter;

			// TODO: locking this entire block is only for running pcm files at highest possible speed
			//daq->ZDAQ_LOCK.unlock();
		}
		else
			usleep(1);
	}

	printf("Closing Detector thread\n");
}


// Time domain leading edge finder
bool find_peak()
{
	// backtrack buffer
	if (tmp_backtrack_buffer != NULL) {
		free(tmp_backtrack_buffer);
	}
	size_t backtrack_buffer_len = N_CHAN * N_FFT * d_factor * 2;
	tmp_backtrack_buffer = (float*) malloc(sizeof(float) * backtrack_buffer_len);

	backtrack_lock.lock();
	// copy history to backtrack buffer
	//memcpy(tmp_backtrack_buffer, &history[0][0], sizeof(float) * N_CHAN * N_FFT * d_factor);
	memcpy(tmp_backtrack_buffer, history, sizeof(float) * N_CHAN * N_FFT * d_factor);
	printf("Beginning find peak\n");
	fflush(stdout);
	// copy current to backtrack buffer
	//memcpy(&tmp_backtrack_buffer[N_CHAN * N_FFT * d_factor], &current[0][0], sizeof(float) * N_CHAN * N_FFT * d_factor);
	memcpy(&tmp_backtrack_buffer[N_CHAN * N_FFT * d_factor], current, sizeof(float) * N_CHAN * N_FFT * d_factor);
	backtrack_lock.unlock();



	int clap_len = 120;
	int window_length = 4800; // 4800 samples @ 48000 sample rate = 100ms
	unsigned int trigger_index = 0;
	double trigger_ratio = 100.0f;

	bool result = false;
	while (!result) {
		// Create rolling average object
		Ravg<double, double> ravg(window_length);

		// Insert initial data
		for (int i = 0; i < window_length; ++i) {
			double sum = 0.0f;
			for (int j = 0; j < N_CHAN; ++j) {
				sum += tmp_backtrack_buffer[N_CHAN*i + j];
			}
			double sample_avg = sum / N_CHAN;
			ravg.insert(sample_avg * sample_avg);
		}

		// step through 1 sample (19 channels) at a time
		for (int i = window_length; i < (backtrack_buffer_len / N_CHAN); ++i) {
			// get the next sample value (average of all 19 channels for the new sample)
			double sample_sum = 0.0f;
			for (int j = 0; j < N_CHAN; ++j) {
				sample_sum += tmp_backtrack_buffer[N_CHAN*i + j];
			}
			double sample_avg = sample_sum / N_CHAN;
			double window_average = ravg.insert(sample_avg * sample_avg);

			// trigger
			if ( (sample_avg * sample_avg) / window_average > trigger_ratio) {
				printf("\nwindow ratio = %.2f\n", (sample_avg * sample_avg) / window_average);
				trigger_index = i;
				printf("trigger_index = %u\n", trigger_index);
				break;
			}
		}

		if (trigger_index < N_FFT*d_factor) {
			//printf("invalid trigger_index = %u\n", trigger_index);
			printf(".");

			trigger_ratio--;
			if (trigger_ratio <= 1) {
				break;
			}
			continue;
		} else {
			result = true;
			break;
		}
	}

	if (!result) {
		printf("No trigger found!\n");
		m->send_angle(-42069, -42069, *detect_ts);
		return false;
	}

	float* trig_ptr = &tmp_backtrack_buffer[ N_CHAN * (trigger_index + clap_len - N_FFT*d_factor) ];

	// zero pad the data
	memset(trig_ptr, 0.0f, sizeof(float) * (N_CHAN * N_FFT * d_factor) - (N_CHAN * d_factor * clap_len));

	// Call the direction finding function
	return find_direction(trig_ptr);
}

// Yamartino Method
bool find_direction(float* data)
{
	double best_angle[2] = {0.0f, 0.0f};
	size_t ang_thresh = p_trig_thresh;

	double x[2] = {0.0f, 0.0f};
	double y[2] = {0.0f, 0.0f};
	size_t N_best = 0;
	double tmp;

	Decimator<float> d(N_CHAN, sampling_freq, cutoff_freq, stopband_ripple, 1, N_FFT*d_factor);

	// FFTer init
	FFTer fft_dir(N_CHAN, N_FFT*d_factor);
	fft_dir.input = d.buffer;
	if (!fft_dir.Initialize()) {
		fprintf(stderr, "Failed to initialize fft_dir\n"); fflush(stderr);
		exit(1);
	}

	// Angler and Powerer init
	Angler angl(N_CHAN, 3, N_bins, angle_res);
	Powerer pwer(N_CHAN, N_bins);
	if(!angl.Initialize(array_geometry, quiet_freqs, bin_width, speed) or !pwer.Initialize(quiet_freqs, bin_width))
	{
		fprintf(stderr,"Failed to initialize\n"); fflush(stderr);
		exit(1);
	}

	d.ReadAll(data);
	fft_dir.FFT();

	pwer.ProcessDataAll(fft_dir.output);
	pwer.PowerCalc();
	for(int b = 0; b < N_bins; ++b)
	{
		if(pwer.quiet_bin[b])
		{
			tmp = pwer.power[b] - stats.mean[b];
			pwer.power_bin[b] = (tmp*tmp > rel_power_thresh*stats.variance[b] and pwer.power[b] > abs_power_thresh);
		}
	}

	angl.ProcessDataAll(fft_dir.output);
	angl.FindAnglesAll();
	for(int b = 0; b < N_bins; ++b)
	{
		if(angl.quiet_bin[b] and pwer.power_bin[b] and angl.err[b] < err_thresh)
		{
			// Conversion to radians and trigonometry
			x[0] += cos(angl.angles[b][0]*0.017453292519943295);
			y[0] += sin(angl.angles[b][0]*0.017453292519943295);
			x[1] += cos(angl.angles[b][1]*0.017453292519943295);
			y[1] += sin(angl.angles[b][1]*0.017453292519943295);
			++N_best;
		}
	}
	if(N_best > ang_thresh)
	{
		// Conversion to degrees
		best_angle[0] = atan2(y[0],x[0])*57.29577951308232;
		best_angle[1] = atan2(y[1],x[1])*57.29577951308232;

		// angle conversion for drone
		while(best_angle[0] > 180)
			best_angle[0] = best_angle[0] - 360;
		while(best_angle[0] < -180)
			best_angle[0] = best_angle[0] + 360;

		// TODO: send the angle message
		m->send_angle(best_angle[0], best_angle[1], *detect_ts);
		printf("Number of angles: %ld\nBest phi: %f\nBest the: %f\n",N_best, best_angle[0], best_angle[1]);
		return true;
	}
	else
	{
		m->send_angle(-42069, -42069, *detect_ts);
		return false;
	}
}

/*
// Tally Method

unsigned int max_tally(unsigned int* tally, unsigned int size, unsigned int &max_index)
{
	unsigned int max = 0;
	for(int i = 0; i < size; ++i)
	{
		if(tally[i] > max)
		{
			max = tally[i];
			max_index = i;
		}
	}
	return max;
}

bool find_direction(float* data)
{
	// Angle calculation
	unsigned int N_phi = ceil(360/angle_res);
	unsigned int N_the = ceil(180/angle_res);
	unsigned int phi_tally[N_phi], the_tally[N_the];
	unsigned int phi_max_index, the_max_index;
	unsigned int phi_max_tally, the_max_tally;
	double tmp;

	memset(phi_tally, 0, sizeof(unsigned int) * N_phi);
	memset(the_tally, 0, sizeof(unsigned int) * N_the);
	phi_max_tally = 0;
	the_max_tally = 0;
	phi_max_index = 0;
	the_max_index = 0;


	// Decimator with d_factor 0 -- for converting float to double
	Decimator<float> d(N_CHAN, sampling_freq, cutoff_freq, stopband_ripple, 1, N_FFT*d_factor);

	// FFTer init
	FFTer fft_dir(N_CHAN, N_FFT*d_factor);
	fft_dir.input = d.buffer;
	if (!fft_dir.Initialize()) {
		fprintf(stderr, "Failed to initialize fft_dir\n"); fflush(stderr);
		exit(1);
	}

	// Angler init
	Angler angl(N_CHAN, 3, N_bins, angle_res);
	if(!angl.Initialize(array_geometry, quiet_freqs, bin_width, speed))
	{
		fprintf(stderr,"Failed to initialize\n"); fflush(stderr);
		exit(1);
	}

	// Powerer init
	Powerer pwer(N_CHAN, N_bins);
	if(!pwer.Initialize(quiet_freqs, bin_width))
	{
		fprintf(stderr,"Failed to initialize\n"); fflush(stderr);
		exit(1);
	}

	int tally_thresh = 40;

	double best_angle = 0.0f;
	int best_tally = 0;

	int try_count = 0;
	int max_tries = 5;
	int offset = 0;
	int offset_interval = 20;

	bool result = false;
	while (!result) {
		d.ReadAll(&data[N_CHAN*offset]);
		fft_dir.FFT();

		pwer.ProcessDataAll(fft_dir.output);
		pwer.PowerCalc();
		for(int b = 0; b < N_bins; ++b)
		{
			if(pwer.quiet_bin[b])
			{
				tmp = pwer.power[b] - stats.mean[b];
				pwer.power_bin[b] = (tmp*tmp > rel_power_thresh*stats.variance[b] and pwer.power[b] > abs_power_thresh);
			}
		}

		angl.ProcessDataAll(fft_dir.output);
		angl.FindAnglesAll();
		for(int b = 0; b < N_bins; ++b)
		{
			if(angl.quiet_bin[b] and pwer.power_bin[b])
			//if(angl.quiet_bin[b] and angl.err[b] < err_thresh)
			{
				//printf("<%3.0f, %3.0f>\t+-%f\n", angl.angles[b][0], angl.angles[b][1], sqrt(angl.err[b]));

				int phi_t_index = angl.angles[b][0] / angle_res;
				int the_t_index = angl.angles[b][1] / angle_res;

				phi_tally[phi_t_index] += 1;
				the_tally[the_t_index] += 1;
			}
		}
		phi_max_tally = max_tally(phi_tally, N_phi, phi_max_index);
		the_max_tally = max_tally(the_tally, N_the, the_max_index);
		printf("ANGLE =>\t <%3.0f, %3.0f>\t<%2.0d, %2.0d>\n", phi_max_index*angle_res, the_max_index*angle_res, phi_max_tally, the_max_tally);

		if (phi_max_tally > best_tally) {
			best_tally = phi_max_tally;
			best_angle = phi_max_index*angle_res;
			result = true;
			break;
		}

		if (best_tally >= tally_thresh) {
			result = true;
			break;
		}

		if (++try_count >= max_tries) {
			result = true;
			break;
		}
		offset -= offset_interval;
	}

	if (!result) {
		printf("No angle found\n");
		m->send_angle(42069, 0.0f, *detect_ts);
		return result;
	}


	// angle conversion for drone
	double output = -1*best_angle;
	while(output > 180)
		output = output - 360;
	while(output < -180)
		output = output + 360;

	// TODO: send the angle message
	m->send_angle(output, 0.0f, *detect_ts);

	printf("Direction Done. Took %d tries\n\n", ++try_count);
	return result;
}
*/


int main(int argc, char* argv[])
{
	algo_buffer = new float [algo_buff_elements];
	algo_head = algo_buffer + N_CHAN*N_FFT*d_factor;
	current = algo_buffer + N_CHAN*ZDAQ_SAMPLES;
	history = algo_buffer + algo_buff_elements - N_CHAN*N_FFT*d_factor;

	// Handle interrupts (ctrl+c)
	add_signal_handlers();

	bool use_zylia = true;

	// Create Zdaq object
	daq = new Zdaq(use_zylia, argv[1], ZDAQ_SAMPLES);

	// TODO: Create messenger object
	m = new Messenger();

	// Create detector thread
	thread dt_thread(detector_thread);

	dt_thread.join();
	daq->stop();

	printf("Closing main thread\n");
	delete algo_buffer;

	return 0;
}
