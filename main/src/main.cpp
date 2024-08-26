//   Includes

#include "main.h"


//   TOP
int
top(int argc, char **argv)
{
	//   PARSE THE COMMANDS
	// Default input arguments
#ifdef __APPLE__
	char *uart_name = (char*)"/dev/tty.usbmodem1";
#else
	char *uart_name = (char*)"/dev/ttyUSB0";
#endif
	int baudrate = 57600;

	bool use_udp = false;
	char *udp_ip = (char*)"127.0.0.1";
	int udp_port = 14540;

	// do the parse, will throw an int if it fails
	parse_commandline(argc, argv, uart_name, baudrate, use_udp, udp_ip, udp_port);


	//   PORT and THREAD STARTUP
	/*
	 * Instantiate a generic port object
	 *
	 * This object handles the opening and closing of the offboard computer's
	 * port over which it will communicate to an autopilot.  It has
	 * methods to read and write a mavlink_message_t object.  To help with read
	 * and write in the context of pthreading, it gaurds port operations with a
	 * pthread mutex lock. It can be a serial or an UDP port.
	 *
	 */
	Generic_Port *port;
	if(use_udp)
	{
		port = new UDP_Port(udp_ip, udp_port);
	}
	else
	{
		port = new Serial_Port(uart_name, baudrate);
	}

	Autopilot_Interface autopilot_interface(port);

	/*
	 * Setup interrupt signal handler
	 *
	 * Responds to early exits signaled with Ctrl-C.  The handler will command
	 * to exit offboard mode if required, and close threads and the port.
	 * The handler in this example needs references to the above objects.
	 *
	 */
	port_quit         = port;
	autopilot_interface_quit = &autopilot_interface;
	signal(SIGINT,quit_handler);

	/*
	 * Start the port and autopilot_interface
	 * This is where the port is opened, and read and write threads are started.
	*/
	port->start();
	autopilot_interface.start();
	main_control(autopilot_interface);
	//   THREAD and PORT SHUTDOWN
	autopilot_interface.stop();
	port->stop();
	delete port;

	//   DONE
	return 0;

}

int main_control(Autopilot_Interface &api){

	// START ZYLIA -> for LED only
	// Set color: icotl(zylia, 0x00000205, <array, 0xBGR>)
	char zylia_path[] = "/dev/zylia-zm-1_0";
	int zylia = zylia_setup(zylia_path);
	const int led_red[1] = {0x0000FF};
	const int led_green[1] = {0x00FF00};
	const int led_white[1] = {0xFFFFFF};


	/* START NAMED PIPES
	 * Named pipe between gui and main
	 * Named pipe between stepper motor and main
	 * Named pipe out to detection
	 */
	// FIFO Paths
	char gui_ifile_name[] = "/tmp/gui_main";
	char gui_ofile_name[] = "/tmp/main_gui";
	char stepper_ifile_name[] = "/tmp/motorFeedback";
	char stepper_ofile_name[] = "/tmp/motorControl";
	char noah_ifile_name[] = "/tmp/noah_angle";
	//char channelvis_ifile_name[] = "/tmp/channelvis_FTM";
	//char channelvis_ofile_name[] = "/tmp/channelvis_MTP";
	//char detection_ifile_name[] = "/tmp/detection_main";
	//char detection_ofile_name[] = "/tmp/main_detection";

	// Pipe In/Out messages
	uint16_t gui_in; // Initialzie, scan, motor
	double gui_out[2]; // battery, current angle
	double stepper_in[2]; // current angle, current action
	double stepper_out[3]; // desire angle, sleep time, hold time
	double noah_in[2];
	//float channelvis_in[1];
	//float channelvis_out[1];
	//bool detec_in = false; // flag
	//bool detec_out = true; // flag

	// Setup
	simplePipe<uint16_t, 1> gui_simpleIn(gui_ifile_name, O_RDONLY);
	simplePipe<double, 2> gui_simpleOut(gui_ofile_name, O_WRONLY);
	simplePipe<double, 2> stepper_simpleIn(stepper_ifile_name, O_RDONLY);
	simplePipe<double, 3> stepper_simpleOut(stepper_ofile_name, O_WRONLY);
	simplePipe<double, 2> noah_simpleIn(noah_ifile_name, O_RDONLY);
	//simplePipe<float, 1> channelvis_simpleIn(channelvis_ifile_name, O_RDONLY);
	//simplePipe<float, 1> channelvis_simpleOut(channelvis_ofile_name, O_WRONLY);
	//simplePipe<bool, 1> detection_simpleIn(detection_ifile_name, O_RDONLY);
	//simplePipe<bool, 1> detection_simpleOut(detection_ofile_name, O_WRONLY);
	bool write = false; // Pipe write flag

	//Harley: Make a new simplePipe for channelvisualiser.py (wont use here)
	//char channelvisualiser_ofile_name[] = "/tmp/channelvis";
	//simplePipe<float, 1> channel_simpleOut(channelvisualiser_ofile_name, O_WRONLY);

	// Stepper Pipe Out
	const double stepper_init[3] = {361, -1, 0}; 
	const double stepper_mid[3] = {177, 0.0050, 1}; 
	const double stepper_right[3] = {132, 0.01, 1}; 
	const double stepper_left[3] = {222, 0.01, 1}; 

	// FSM 
	uint8_t cur_state = 0;
	uint8_t next_state = 0;
	sys_status status;

	double checkpoint;
	printf("\nDRONE DEMO START\n");

	while(true){
		// FIFO read
		while(true){
			int read_stepper = stepper_simpleIn.pipeIn((double (&)[2])stepper_in);
			int read_gui = gui_simpleIn.pipeIn((uint16_t (&)[1])gui_in);
			int read_detect = noah_simpleIn.pipeIn((double (&)[2])noah_in);
			//int read_channelvis = channelvis_simpleIn.pipeIn((float(&)[1])channelvis_in);

			if(read_gui > 0){
				status = gui_message_handler(api, gui_in, status, cur_state);
				//printf("Out: %d %d %d\n", status.init, status.scan, status.motor);
				break;
			}
			if(read_detect > 0 && status.detect){
				//TODO
				write = signal_detection(zylia, noah_in[0], stepper_in[0], checkpoint, stepper_out);
				if(write) break;
			}
			if(read_stepper > 0 && stepper_in[1] <= 0) {
				printf("Goal reach\n");
				cur_state = next_state;
				break;
			}
			/*if (read_channelvis > 0) {
				channelvis_out = channelvis_in;
				channelvis_simpleOut.pipeOut((float(&)[1]) channelvis_out);
			}
			else {
				channelvis_out[0] = 0;
				channelvis_simpleOut.pipeOut((float(&)[1]) channelvis_out);
			}*/
			usleep(1000);
		}
		//printf("Current angle: %f Current action: %f\n", stepper_in[0], stepper_in[1]);
		if(!write){
			switch(cur_state){
				case 0:
					if (!status.init) break;
					printf("INITIALIZING...\n");
					ioctl(zylia, 0x00000205, led_white);
					std::copy(std::begin(stepper_init), std::end(stepper_init), std::begin(stepper_out));
					write = true;
					next_state = 1;
					break;
				case 1:
					printf("GOTO MIDDLE\n");
					std::copy(std::begin(stepper_mid), std::end(stepper_mid), std::begin(stepper_out));
					status.init = false;
					write = true;
					next_state = 2;
					break;
				case 2:
					if (!status.scan) break;
					printf("GOTO RIGHT\n");
					ioctl(zylia, 0x00000205, led_green);
					std::copy(std::begin(stepper_right), std::end(stepper_right), std::begin(stepper_out));
					write = true;
					next_state = 3;
					break;
				case 3:
					if (!status.scan) break;
					printf("GOTO LEFT\n");
					ioctl(zylia, 0x00000205, led_green);
					std::copy(std::begin(stepper_left), std::end(stepper_left), std::begin(stepper_out));
					write = true;
					next_state = 2;
					break;
				default:
					break;
			}
		}

		// FIFO write
		if(write){
			// Write to stepper
			int write_bytes = stepper_simpleOut.pipeOut((double (&)[3])stepper_out);
			while(write_bytes <= 0){
				write_bytes = stepper_simpleOut.pipeOut((double (&)[3])stepper_out);
				printf("Reattempting to write\n");
			}
			write = false;
			//printf("Writing: %f %f %f\n", stepper_out[0], stepper_out[1], stepper_out[2]);
		}
		usleep(1000);
	}
	
	return 0;

};


// Detection scheme
bool signal_detection(int zylia, double detect_angle, double cur_angle, double &checkpoint, double (&stepper_out)[3]){
	double d_angle = detect_angle;
	const int led_red[1] = {0x0000FF};
	const int led_yellow[1] = {0x00FFFF};
	const int led_green[1] = {0x00FF00};
	if(d_angle == -42069){
		printf("Error: choosing to ignore.\n");
		ioctl(zylia, 0x00000205, led_green);
		stepper_out[0] = 177;
		stepper_out[1] = 0.01;
		stepper_out[2] = 1;
		return true;
	}
	if(d_angle == 42069){
		printf("Waiting for a trigger\n");
		ioctl(zylia, 0x00000205, led_yellow);
		stepper_out[0] = cur_angle;
		stepper_out[1] = 42069;
		stepper_out[2] = 42069;
		checkpoint = cur_angle;
		return true;
	}
	if (fabs(d_angle) <= 180){
		ioctl(zylia, 0x00000205, led_red);
		d_angle = fmod((checkpoint + (360 + d_angle)), 360.0);
		printf("RECEIVED -> Angle: %f\n", d_angle);
		stepper_out[0] = d_angle;
		stepper_out[1] = 0.0045;
		stepper_out[2] = 5;
		return true;
	}
}
// Handle gui interface
sys_status gui_message_handler(Autopilot_Interface &api, uint16_t gui_in, sys_status cur_status, uint8_t &cur_state){
        sys_status status = cur_status;
        switch(gui_in){
                case 1:
                        play_tune(api, 0);
                        cur_state = 0;
                        status.init = true;
                        status.scan = false;
                        status.motor = false;
                        motor_start(api, status.motor);
                        break;
                case 2:
                        if (cur_state < 2) break;
                        play_tune(api, 1);
                        status.scan = !status.scan;
			status.detect = !status.detect;
                        if(status.scan == false) cur_state = 1;
                        //printf("SCAN: %d\n", status.scan);
                        break;
                case 3:
                        if (cur_state < 2) break;
                        play_tune(api, 1);
                        status.motor = !status.motor;
                        motor_start(api, status.motor);
                        break;
                default:
                        break;
        }
        return status;
}


// PLAY TUNE
void play_tune(Autopilot_Interface &api, double tune_id)
{
	// 0: initialize
	// 1: button pressed
	// 2: low battery
	char* tune;
	printf("PLAY TUNE %d\n", (int)tune_id);
	switch((uint8_t)tune_id){
		case 0:
			tune = "MFT200L16<cdefgab>cdefgab.c";
			api.play_tune(tune);
			break;
		case 1: 
			tune = "MFL32<<<G";
			api.play_tune(tune);
			break;
		case 2:
			tune = "MFL4<<<<A";
			for (int i=0;i<3;i++){
				api.play_tune(tune);
				this_thread::sleep_for(chrono::milliseconds(500));
			}
			break;
		default:
			break;
	}
}

//  MOTOR TEST 
void motor_start(Autopilot_Interface &api, bool motors)
{
	// Legs up/down
	// Motor test On/Off
	if (motors){
		api.landing_gear(true);
		for (int i = 0; i < 5; i++){
			api.motor_start(true, i);
			this_thread::sleep_for(chrono::milliseconds(5));
		}
	}
	else {
		api.landing_gear(false);
		for (int i = 0; i < 5; i++){
			api.motor_start(false,i);
			this_thread::sleep_for(chrono::milliseconds(10));
		}
	}
}

//   Parse Command Line
// throws EXIT_FAILURE if could not open the port
void
parse_commandline(int argc, char **argv, char *&uart_name, int &baudrate,
		bool &use_udp, char *&udp_ip, int &udp_port)
{

	// string for command line usage
	const char *commandline_usage = "usage: mavlink_control [-d <devicename> -b <baudrate>] [-u <udp_ip> -p <udp_port>] [-a ]";

	// Read input arguments
	for (int i = 1; i < argc; i++) { // argv[0] is "mavlink"

		// Help
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			printf("%s\n",commandline_usage);
			throw EXIT_FAILURE;
		}

		// UART device ID
		if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--device") == 0) {
			if (argc > i + 1) {
				i++;
				uart_name = argv[i];
			} else {
				printf("%s\n",commandline_usage);
				throw EXIT_FAILURE;
			}
		}

		// Baud rate
		if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--baud") == 0) {
			if (argc > i + 1) {
				i++;
				baudrate = atoi(argv[i]);
			} else {
				printf("%s\n",commandline_usage);
				throw EXIT_FAILURE;
			}
		}

		// UDP ip
		if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--udp_ip") == 0) {
			if (argc > i + 1) {
				i++;
				udp_ip = argv[i];
				use_udp = true;
			} else {
				printf("%s\n",commandline_usage);
				throw EXIT_FAILURE;
			}
		}

		// UDP port
		if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
			if (argc > i + 1) {
				i++;
				udp_port = atoi(argv[i]);
			} else {
				printf("%s\n",commandline_usage);
				throw EXIT_FAILURE;
			}
		}
	}
	// end: for each input argument

	// Done!
	return;
}

// SETUP Zylia
int zylia_setup(char zylia_path[]){
	int fd = open(zylia_path, O_RDWR | O_NONBLOCK);
	while (fd < 0){
		printf("Waiting for Zylia...\n");
		fd = open(zylia_path, O_RDWR | O_NONBLOCK);
		sleep_for(milliseconds(10));
	}

	const int led_manual[1] = {1};
	const int led_white[1] = {0xFFFFFF};
	const int setmode = 0x00000203;
	const int setcolor = 0x00000205;
	ioctl(fd, setmode, led_manual);
	ioctl(fd, setcolor, led_white);
	return fd;
}


//   Quit Signal Handler
// this function is called when you press Ctrl-C
void
quit_handler( int sig )
{
	printf("\n");
	printf("TERMINATING AT USER REQUEST\n");
	printf("\n");

	// autopilot interface
	try {
		autopilot_interface_quit->handle_quit(sig);
	}
	catch (int error){}

	// port
	try {
		port_quit->stop();
	}
	catch (int error){}

	// end program here
	exit(0);

}



int
main(int argc, char **argv)
{       
	// This program uses throw, wrap one big try/catch here
	try
	{
		int result = top(argc,argv);
		return result;
	}
		
	catch ( int error )
	{
		fprintf(stderr,"mavlink_control threw exception %i \n" , error);
		return error;
	}

}

