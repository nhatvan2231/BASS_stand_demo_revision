#include "zdaq.h"

Zdaq::Zdaq(int input_type, char* source_str, int N_samples)
{
	this->input_type = input_type;
	if(N_samples > 0)
		n_samples = N_samples;
	else
		n_samples = DEFAULT_NUM_SAMPLES;

	n_channels = DEFAULT_NUM_CHANNELS;

	int err = 0;

	// Initialize the input device
	if (input_type) {
		err = usb_init(source_str);
		if (err != 0) {
			// exit
			printf("Could not initialize Zylia device\n");
			exit(1);
		} else {
			printf("Zylia Initialized\n");
		}
	} else {
		err = pcm_init(source_str);
		if (err != 0) {
			// exit
			printf("Could not initialize PCM reader\n");
			exit(1);
		} else {
			printf("PCM initialized\n");
		}
	}

	// Allocate memory for the reader
	read_buffer_elements = n_samples * n_channels * 10;						// TODO: hardcoded 5 value
	read_buffer_size = sizeof(float) * read_buffer_elements;
	READ_BUFFER = (float*) malloc(read_buffer_size);
	if (!READ_BUFFER) {
		// exit
		printf("Could not allocate memory for reader\n");
		exit(1);
	}

	ZDAQ_IS_READ = false;
	ZDAQ_IS_DONE = false;
}

// TODO: destructor

// Initialize USB input
int Zdaq::usb_init(char* custom_device)
{
	int err;
	char device[64];
	memset(device, 0, sizeof(device));
	if (custom_device != NULL) {
		strcpy(device,custom_device);
	} else {
		strcpy(device, DEFAULT_ZYLIA_DEVICE);
	}

	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0);
	if (err < 0) {
		printf("snd_pcm_open fail: %d: %s (%s)\n", err, snd_strerror(err), device);
		exit(1);
	}

	// Initialize Params
	snd_pcm_hw_params_t *hw_params;

	err = snd_pcm_hw_params_malloc(&hw_params);
	if (err < 0) {
		printf("snd_pcm_hw_params_malloc fail: %d: %s\n", err, snd_strerror(err));
		exit(1);
	}

	err = snd_pcm_hw_params_any(handle, hw_params);
	if (err < 0) {
		printf("snd_pcm_hw_params_any fail: %d: %s\n", err, snd_strerror(err));
		exit(1);
	}

	err = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		printf("snd_pcm_hw_params_set_access fail: %d: %s\n", err, snd_strerror(err));
		exit(1);
	}

	err = snd_pcm_hw_params_set_format(handle, hw_params, DEFAULT_ZYLIA_FORMAT);
	if (err < 0) {
		printf("snd_pcm_hw_params_set_format fail: %d: %s\n", err, snd_strerror(err));
		exit(1);
	}

	err = snd_pcm_hw_params_set_rate(handle, hw_params, DEFAULT_RATE, 0);
	if (err < 0) {
		printf("snd_pcm_hw_params_set_rate fail: %d: %s\n", err, snd_strerror(err));
		exit(1);
	}

	err = snd_pcm_hw_params_set_channels(handle, hw_params, n_channels);
	if (err < 0) {
		printf("snd_pcm_hw_params_set_channels fail: %d: %s\n", err, snd_strerror(err));
		exit(1);
	}

	err = snd_pcm_hw_params(handle, hw_params);
	if (err < 0) {
		printf("snd_pcm_hw_params fail: %d: %s\n", err, snd_strerror(err));
		exit(1);
	}

	snd_pcm_hw_params_free(hw_params);

	err = snd_pcm_prepare(handle);
	if (err < 0) {
		printf("snd_pcm_prepare fail: %d: %s\n", err, snd_strerror(err));
		exit(1);
	}
	return 0;
}

// Initialize PCM input
int Zdaq::pcm_init(char* directory)
{
	if (directory == NULL) {
		printf("Error: pass a directory with pcm files as argument\n");
		exit(1);
	}

	for(auto &entry : std::filesystem::directory_iterator(directory)) {
		// check that the path ends with .pcm
		std::string path_str(entry.path());
		if ( path_str.compare(path_str.length() - 4, 4, ".pcm") == 0 ) {
			sorted_by_name.insert(entry.path());
		}
	}

	if (sorted_by_name.size() == 0) {
		printf("Directory has no .pcm files!\n");
		return 1;
	}

	return 0;
}


// Starts the data acquisition thread
int Zdaq::start()
{
	// begin read_thread
	thread_obj = new std::thread(&Zdaq::read_thread, this);
	return 0;
}

// Stops the data acquisition thread
int Zdaq::stop()
{
	ZDAQ_IS_DONE = true;
	thread_obj->join();
	return 0;
}

// Returns struct with read info
Zdaq::zdaq_read_t Zdaq::read()
{
	return read_status;
}

void Zdaq::read_thread()
{
	int err;
	bool is_read_flag = false;					// used for handling read size less than FRAMES_PER_READ when reading PCM files
	int frames_read = 0;

	// pointer to the location in the buffer to write new frames to
	float* buff_ptr = READ_BUFFER;

	// open file if pcm input
	std::set<std::filesystem::path>::iterator file_iterator;
	if (!input_type) {
		file_iterator = sorted_by_name.begin();
		pcm_file = fopen(file_iterator->c_str(), "r");
	}

	ZDAQ_IS_DONE = false;
	while (!ZDAQ_IS_DONE) {
		// check if we are at the end of the buffer, wrap around if so
		if ( buff_ptr >= (READ_BUFFER + read_buffer_elements) ) {
			buff_ptr = READ_BUFFER;
		}

		// read from the correct device, usb or pcm
		if (input_type) {
			// usb
			ZDAQ_LOCK.lock();
			err = snd_pcm_readi(handle, buff_ptr, FRAMES_PER_READ);
			ZDAQ_LOCK.unlock();
			if (err != FRAMES_PER_READ) {
				printf("snd_pcm_readi fail: %d: %s\n", err, snd_strerror(err));
				break;
			} else {
				frames_read += FRAMES_PER_READ;
			}
		} else {
			// pcm
			//usleep(1334); // simulate zylia read delay

			ZDAQ_LOCK.lock();
			err = fread(buff_ptr, sizeof(float)*n_channels, FRAMES_PER_READ, pcm_file);
			ZDAQ_LOCK.unlock();

			frames_read += err;
			if (feof(pcm_file)) {
				printf("ZDAQ: Reached EOF\n");
				// close current file
				fclose(pcm_file);

				// open next file
				file_iterator++;
				if (file_iterator != sorted_by_name.end()) {
					pcm_file = fopen(file_iterator->c_str(), "r");
				} else {
					break;
				}
			}
		}

		// move the ptr to the next position
		buff_ptr += err * n_channels;

		// set ZDAQ_IS_READ once we have read enough frames
		//if ( frames_read == n_samples || is_read_flag ) {
		if ( frames_read == n_samples || is_read_flag ) {
			// update read_status struct
			read_status.num_elems = n_channels*frames_read;
			read_status.buff_ptr = buff_ptr - (n_samples*n_channels);

			ZDAQ_IS_READ = true;
			frames_read = 0;
		}
	}

	// close the input device, usb or pcm
	if (input_type) {
		snd_pcm_close(handle);
	} else {
		if (pcm_file) {
			fclose(pcm_file);
		}
	}

	printf("Exiting read_thread\n");
	ZDAQ_IS_DONE = true;
}
