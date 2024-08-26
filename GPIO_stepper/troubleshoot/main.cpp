#include <unistd.h> // usleep()
#include <stdio.h> // fprintf()
#include <cstdlib> // atoi()

#include <pigpiod_if2.h> // GPIO related functions
#include "simple_pipe.h" // simplePipe()

#define IP_ADDRESS NULL
#define PORT NULL
#define MID 177 // Degrees to get to middle (facing forward)
// Note:
// - FIFO File requires two data point inputs:
//    - (double, int)
//    - desired angle (0 to 360) degrees and time in microseconds (0 to MAX_INT) between steps
//       - values above the range mean do not perform that action
//       - values below the range mean will exit

int angleRoutine(simplePipe<double, 2> angleAndSleep, int handle, int gpioLim, int gpioDir, int gpioPul);
int stepTowards(int handle, int gpio_dir, int gpio_pulse, bool CCW, int& position);
int pulse(int handle, int gpio);

int main(int argc, char* argv[]) {
	char* fifo_filename;

	int GPIO_lim;	// limit switch
	int GPIO_dir;	// direction
	int GPIO_pul;	// step

	if(argc == 2) {
		fifo_filename = argv[1];
	}
	else if(argc == 5) {
		fifo_filename = argv[1];
		GPIO_lim = atoi(argv[2]);
		GPIO_dir = atoi(argv[3]);
		GPIO_pul = atoi(argv[4]);
	}
	else { // defaults
		char tmp[] = "/tmp/angle_speed";
		GPIO_lim = 15;
		GPIO_dir = 12;
		GPIO_pul = 18;
		fifo_filename = tmp;
	}

	// Set up named pipe
	simplePipe<double, 2> angleAndSleep(fifo_filename, O_RDONLY);

	// Initialize GPIO
	int pi = pigpio_start(IP_ADDRESS, PORT);
	if(pi < 0) {
		fprintf(stderr, "Cannot initialize GPIO");
		return -1;
	}
	printf("GPIO successfully initialized\n");

	//// Claim GPIO for limit switch
	if(set_mode(pi, GPIO_lim, PI_INPUT) < 0) {
		fprintf(stderr, "Cannot claim GPIO: %d for input\n", GPIO_lim);
		return -1;
	}
	printf("Limit switch successfully initialized\n");

	//// Claim GPIO for direction and pulse
	if(set_mode(pi, GPIO_dir, PI_OUTPUT) < 0 || set_mode(pi, GPIO_pul, PI_OUTPUT) < 0) {
		fprintf(stderr, "Cannot claim GPIO: %d and %d for outpu\n", GPIO_dir, GPIO_pul);
		return -1;
	}
	printf("GPIO for motor successfully initialized\n");

	angleRoutine(angleAndSleep, pi, GPIO_lim, GPIO_dir, GPIO_pul);

	pigpio_stop(pi);
	return 0;
}

int angleRoutine(simplePipe<double, 2> angleAndSleep, int handle, int gpioLim, int gpioDir, int gpioPul) {
	int readBytes = 0;
	int readPeriod = 5000; // TODO Workout how to incorporate this to include checks
	double message[4] = {369,369,369,369};
	double* m_head = message;
	double* m_tail = message + 2;

	int stepPosition = 0;
	const int stepMax = 1200; // number of steps that correspond to a full 360 degrees
	const double degrees_to_steps = 360.0/stepMax; // conversion from degrees to steps

	//TODO do this with signal handlers
	while(true) {
		readBytes = angleAndSleep.pipeIn((double(&)[2]) m_head);
		if(readBytes > 0) {
			if (m_head[0] < 0 && m_head[1] < 0)
				break; // Negatives cause a break and exit

			else if (m_head[0] > 360 && m_head[1] < 0) {
				// TODO implement initialization
				// TODO Put go to limit switch here
				while(gpio_read(handle, gpioLim) != 1)
					stepTowards(handle, gpioDir, gpioPul, false, stepPosition);

				stepPosition = 0;
				m_head[0] = MID;
			}
			m_tail = m_head; // moves tail to newly updated angle
			m_head = message + 2 * (m_head == message + 2); // prepares array to read in next message
		}

		// Starts stepping towards angle
		else if(0 < m_tail[0] && m_tail[0] < 360) {
			int goal = m_tail[0] * degrees_to_steps;
			int diff = goal - stepPosition;

			// Goes immediately to angle
			/*
			while(stepPosition < abs(m_tail[0] * degrees_to_steps)){
				stepTowards(handle, gpioDir, gpioPul, diff > 0, stepPosition);
				usleep((int) m_tail[1]);
			}
			*/

			// Goes N_steps towards angle then checks simplePipe
			int N_steps = readPeriod/m_tail[1];
			for(int i = 0; i < N_steps; ++i) {
				if(stepPosition == goal) break;

				stepTowards(handle, gpioDir, gpioPul, diff > 0, stepPosition);
				usleep((int) m_tail[1]);
			}
		}
	}
	return 0;
}


int stepTowards(int handle, int gpio_dir, int gpio_pulse, bool CCW, int& position) {
	int err = 0;
	// Set direction
	if(gpio_read(handle, gpio_dir) == CCW) {
		err = gpio_write(handle, gpio_dir, CCW);
		if(err < 0) {
			fprintf(stderr, "Cannot write to GPIO: %d\n", gpio_dir);
			return err;
		}
	}
	// Step in that direction
	err = pulse(handle, gpio_pulse);
	if(err < 0) {
		fprintf(stderr, "Cannot write to GPIO: %d\n", gpio_pulse);
		return err;
	}
	// Adjusts position depending on direction
	position += 2*CCW - 1;
	return 0;
}

int pulse(int handle, int gpio) {
	int err = 0;

	err = gpio_write(handle, gpio, 1);
	if(err < 0) {
		fprintf(stderr, "Cannot write to GPIO: %d\n", gpio);
		return err;
	}

	err = gpio_write(handle, gpio, 0);
	if(err < 0) {
		fprintf(stderr, "Cannot write to GPIO: %d\n", gpio);
		return err;
	}

	return err;
}
