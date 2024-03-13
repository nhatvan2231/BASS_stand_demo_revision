#ifndef __ZDAQ_H__
#define __ZDAQ_H__

#include <atomic>
#include <set>
#include <thread>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <mutex>
#include <alsa/asoundlib.h>

#define DEFAULT_NUM_SAMPLES 40960
#define DEFAULT_NUM_CHANNELS 19
#define DEFAULT_RATE 48000

#define FRAMES_PER_READ 64

#define DEFAULT_ZYLIA_DEVICE "plughw:CARD=ZM13E,DEV=0"
#define DEFAULT_ZYLIA_FORMAT (SND_PCM_FORMAT_FLOAT_LE)

class Zdaq
{
public:
	size_t read_buffer_elements;
	size_t read_buffer_size;
	float* READ_BUFFER;

	std::mutex ZDAQ_LOCK;

	// flags
	std::atomic_bool ZDAQ_IS_READ;
	std::atomic_bool ZDAQ_IS_DONE;

	struct zdaq_read_t {
		size_t num_elems;  // number of elements read
		float* buff_ptr;   // pointer to the most recent sample in the buffer
	};


	// Constructors
	Zdaq(int, char*, int);

	// Function prototypes
	int start();
	int stop();
	zdaq_read_t read();

private:
	int n_channels, n_samples;
	int input_type;

	//bool thread_run = false;
	std::thread* thread_obj;

	snd_pcm_t *handle;                                      // for USB
	FILE* pcm_file;                                         // for PCM
	std::set<std::filesystem::path> sorted_by_name;         // for PCM

	struct zdaq_read_t read_status;

	// Function prototypes
	int usb_init(char*);
	int pcm_init(char*);
	void read_thread();


};


#endif /* __ZDAQ_H__ */
