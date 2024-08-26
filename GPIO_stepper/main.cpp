#include <stdio.h> // fprintf()
#include <cstdlib> // atoi()

#include <pigpiod_if2.h> // GPIO related functions
#include "simplePipe.h" // simplePipe()

#define IP_ADDRESS NULL
#define PORT NULL
#define MID 176 // (Degrees) distance to get to middle (facing forward)
#define READ_PERIOD 0.1 // (Seconds) Time between reads AND minimum time between steps
// Note:
// - FIFO File input requires two data point inputs:
//    - (double, double)
//    - desired angle [0 to 360) degrees and time in seconds [0 to READ_PERIOD] between steps
//       - values above desired angle range and below time between steps range will initialize the stepper
//       - values above the range mean do not perform that action
//       - values below the range mean will exit
// - FIFO File output gives two data point outputs:
//    - (double, double)
//    - an action message [0 to 2] and the current angle [0 to 360)
//       - 0) No action
//       - 1) Initialization
//       - 2) Moving to angle
//       - 3) Hold at angle

#ifdef FIFO_OUTPUT
int angleRoutine(simplePipe<double, 3> inputAngleAndSleep, simplePipe<double,2> outputAngleAndAction, int handle, int gpioLim, int gpioDir, int gpioPul);
#else
int angleRoutine(simplePipe<double, 3> inputAngleAndSleep, int handle, int gpioLim, int gpioDir, int gpioPul);
#endif
int stepTowards(int handle, int gpio_dir, int gpio_pulse, bool CCW, int& position);
int pulse(int handle, int gpio);

int main(int argc, char* argv[]) {
	char* fifo_filename_input;
	char* fifo_filename_output;

	int GPIO_lim;	// limit switch
	int GPIO_dir;	// direction
	int GPIO_pul;	// step

	if(argc == 3) {
		fifo_filename_input = argv[1];
		fifo_filename_output = argv[2];
		GPIO_lim = 22; //37
		GPIO_dir = 18;
		GPIO_pul = 24;
	}
	else if(argc == 6) {
		fifo_filename_input = argv[1];
		fifo_filename_output = argv[2];
		GPIO_lim = atoi(argv[3]);
		GPIO_dir = atoi(argv[4]);
		GPIO_pul = atoi(argv[5]);
	}
	else { // defaults
		char tmp[] = "/tmp/motorControl";
		char tmp2[] = "/tmp/motorFeedback";
		//GPIO_lim = 22;
		GPIO_lim = 22; //37
		GPIO_dir = 18;
		GPIO_pul = 24;
		fifo_filename_input = tmp;
		fifo_filename_output = tmp2;
	}

	// Set up named pipe
	//printf("Waiting for read from %s\t", fifo_filename_output); fflush(stdout);
#ifdef FIFO_OUTPUT
	simplePipe<double, 2> outputAngleAndAction(fifo_filename_output, O_WRONLY);
#endif
	//printf("Done\n");
	//printf("Waiting for write to %s\t", fifo_filename_input); fflush(stdout);
	simplePipe<double, 3> inputAngleAndSleep(fifo_filename_input, O_RDONLY);
	//printf("Done\n");

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
	set_pull_up_down(pi, GPIO_lim, PI_PUD_DOWN);
	printf("Limit switch successfully initialized\n");

	//// Claim GPIO for direction and pulse
	if(set_mode(pi, GPIO_dir, PI_OUTPUT) < 0 || set_mode(pi, GPIO_pul, PI_OUTPUT) < 0) {
		fprintf(stderr, "Cannot claim GPIO: %d and %d for output\n", GPIO_dir, GPIO_pul);
		return -1;
	}
	gpio_write(pi, GPIO_dir, 1);
	gpio_write(pi, GPIO_pul, 1);
	//set_pull_up_down(pi, GPIO_dir, PI_PUD_UP);
	//set_pull_up_down(pi, GPIO_pul, PI_PUD_UP);
	printf("GPIO for motor successfully initialized\n");

#ifdef FIFO_OUTPUT
	angleRoutine(inputAngleAndSleep, outputAngleAndAction, pi, GPIO_lim, GPIO_dir, GPIO_pul);
#else
	angleRoutine(inputAngleAndSleep, pi, GPIO_lim, GPIO_dir, GPIO_pul);
#endif

	pigpio_stop(pi);
	return 0;
}

#ifdef FIFO_OUTPUT
int angleRoutine(simplePipe<double, 3> inputAngleAndSleep, simplePipe<double, 2> outputAngleAndAction, int handle, int gpioLim, int gpioDir, int gpioPul) {
#else
int angleRoutine(simplePipe<double, 3> inputAngleAndSleep, int handle, int gpioLim, int gpioDir, int gpioPul) {
#endif
	int readBytes = 0;
	double input_message[6] = {0,0,0,0,0,0};
	double* m_head = input_message;
	double* m_tail = input_message + 3;
#ifdef FIFO_OUTPUT
	double output_message[2] = {-1,0}; // { -1 (no angle), 0 (no action)}
#endif

	int stepPosition = 0;
	const int stepMax = 1200; // number of steps that correspond to a full 360 degrees
	const double degrees_to_steps = stepMax/360.0; // conversion from degrees to steps

	// hold counter for hold at angle
	volatile int hold_counter = 0;

//	// Initializes on startup
//	//// Note: this write to the fifo output blocks until output is read
//	output_message[0] = -1; // Sets the angle message to NA (degrees)
//	output_message[1] = 1; // Sets the action message to 1 (initialization)
//	outputAngleAndAction.pipeOut((double(&)[2])output_message[0]);
//
//	while(gpio_read(handle, gpioLim) != 0) {
//		stepTowards(handle, gpioDir, gpioPul, false, stepPosition);
//		time_sleep(0.0075); // Delay between Steps
//		printf("Limit switch: %d\n", gpio_read(handle, gpioLim));
//	}
//	time_sleep(1); // delay after hitting limit switch
//	stepPosition = 0; // sets zero point for stepper motor
//
//	output_message[0] = 0; // Sets the angle message to 0 (degrees)
//	output_message[1] = 0; // Sets the action message to 0 (no action)
//	outputAngleAndAction.pipeOut((double(&)[2])output_message[0]);


	//TODO do this with signal handlers
#ifdef FIFO_OUTPUT
		outputAngleAndAction.pipeOut((double(&)[2])output_message[0]);
#endif
	while(true) {
		readBytes = inputAngleAndSleep.pipeIn((double(&)[3])m_head[0]);
		if(readBytes > 0) {
			if (m_head[0] < 0 && m_head[1] < 0)
				break; // Negatives cause a break and exit
			else if (m_head[0] > 360 && m_head[1] < 0) {
				// Re-initializes when given this command
#ifdef FIFO_OUTPUT
				output_message[0] = 0; // Sets the angle message to 0 (degrees)
				output_message[1] = 1; // Sets the action message to 1 (initialization)
				outputAngleAndAction.pipeOut((double(&)[2])output_message[0]);
#endif

				while(gpio_read(handle, gpioLim) != 1) {
#ifdef FIFO_OUTPUT
					outputAngleAndAction.pipeOut((double(&)[2])output_message[0]);
#endif
					printf("Initializing\n");
					stepTowards(handle, gpioDir, gpioPul, false, stepPosition);
					time_sleep(0.0075); // Delay between Steps
					printf("Limit switch: %d\n", gpio_read(handle, gpioLim));
				}
				time_sleep(1); // delay after hitting limit switch
				stepPosition = 0;

#ifdef FIFO_OUTPUT
				output_message[0] = 0; // Sets the angle message to 0 (degrees)
				output_message[1] = 0; // Sets the action message to 0 (no action)
				outputAngleAndAction.pipeOut((double(&)[2])output_message[0]);
#endif

				/* Goes to center after re-initializes
				 * TODO: maybe put this back in?
				*/
				//m_head[0] = MID; // Absolute Angle to turn to
				//m_head[1] = 0.0075; // Delay between Steps

			}
			printf("Received Bytes: %d, Angle: %f, Sleep Time: %f\n", readBytes, m_head[0], m_head[1]); fflush(stdout);
			hold_counter = 0;
			if (m_head[2] >= READ_PERIOD)
				hold_counter = m_head[2]/READ_PERIOD;
			double *tmp = m_tail;
			m_tail = m_head; // moves tail to newly updated angle
			m_head = tmp;
		}

		// Starts stepping towards absolute angle
		if(0 <= m_tail[0] && m_tail[0] < 360) {
#ifdef FIFO_OUTPUT
			output_message[0] = stepPosition/degrees_to_steps; // Sets the angle message to current angle (degrees)
			output_message[1] = 2; // Sets the action message to 2 (moving to angle)
			outputAngleAndAction.pipeOut((double(&)[2])output_message[0]);
#endif

			int goal = m_tail[0] * degrees_to_steps;
			int diff = goal - stepPosition;

			// hold at angle

			// Goes N_steps towards angle then checks simplePipe
			int N_steps = 1;
			if(READ_PERIOD > m_tail[1])
				N_steps = READ_PERIOD/m_tail[1];

			for(int i = 0; i < N_steps; ++i) {
				if(stepPosition == goal) {
					time_sleep(READ_PERIOD); // pause after arriving at goal

#ifdef FIFO_OUTPUT
					//output_message[0] = stepPosition/degrees_to_steps; // Sets the angle message to current angle (degrees)
					if (hold_counter > 0){
						output_message[1] = 3;
						hold_counter = hold_counter - 1;
						//printf("HOLDING: %d\n", hold_counter);
					}
					else{
						output_message[1] = 0; // Sets the action message to 0 (no action)
						m_tail[0] += 360; // Signals to not go back into this
					}
					//printf("Sending confirmation that microphone is at angle of %f\n",output_message[0]);
					outputAngleAndAction.pipeOut((double(&)[2])output_message[0]);
#endif
					break;
				}

				stepTowards(handle, gpioDir, gpioPul, diff > 0, stepPosition);

				double sleepTime = READ_PERIOD;
				// Normal wait time between steps
				if(READ_PERIOD > m_tail[1])
					sleepTime = m_tail[1];

				// Additional deceleration time between steps
				// TODO: Figure out acceleration profile
				if( stepPosition < goal && stepPosition > goal - 5.0/sleepTime)
					sleepTime += (1 + stepPosition/goal) * sleepTime;
				else if( stepPosition > goal && stepPosition < goal + 5.0/sleepTime)
					sleepTime += (1 + goal/stepPosition) * sleepTime;

				time_sleep(sleepTime);
			}
			//printf("Goal %d \t Diff %d\n", goal, diff);
		}
	}
	return 0;
}


int stepTowards(int handle, int gpio_dir, int gpio_pulse, bool CCW, int& position) {
	int err = 0;
	// Set direction
	if(gpio_read(handle, gpio_dir) != CCW) {
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
	time_sleep(0.001);

	err = gpio_write(handle, gpio, 0);
	if(err < 0) {
		fprintf(stderr, "Cannot write to GPIO: %d\n", gpio);
		return err;
	}
	time_sleep(0.001);

	return err;
}
