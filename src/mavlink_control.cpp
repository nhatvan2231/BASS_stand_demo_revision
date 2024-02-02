/****************************************************************************
 *
 *   Copyright (c) 2014 MAVlink Development Team. All rights reserved.
 *   Author: Trent Lukaczyk, <aerialhedgehog@gmail.com>
 *           Jaycee Lock,    <jaycee.lock@gmail.com>
 *           Lorenz Meier,   <lm@inf.ethz.ch>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file mavlink_control.cpp
 *
 * @brief An example offboard control process via mavlink
 *
 * This process connects an external MAVLink UART device to send an receive data
 *
 * @author Trent Lukaczyk, <aerialhedgehog@gmail.com>
 * @author Jaycee Lock,    <jaycee.lock@gmail.com>
 * @author Lorenz Meier,   <lm@inf.ethz.ch>
 *
 */



//   Includes

#include "mavlink_control.h"

struct status{
	bool init = 0; // initialize flags
	bool scan = 0;   // scan
	bool motors = 0;	// motor test start 
};

struct message{
	double msg1;
	double msg2;
};

struct detection_message{
	double detect;
	double detect_angle;
};

//   TOP
int
top (int argc, char **argv)
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

	// START ZYLIA
	// Set color: icotl(zylia, 0x00000205, <array, 0xBGR>)
	char zylia_path[] = "/dev/zylia-zm-1_0";
	int zylia = zylia_setup(zylia_path);

	/* START NAMED PIPES
	 * Named pipe between gui and main
	 * Named pipe between stepper motor and main
	 */
	message gui_in;
	message gui_out;
	message stepper_in;
	message stepper_out;
	message detection_in;
	status control_status;

	char gui_ifile_name[] = "/tmp/gui_main";
	char gui_ofile_name[] = "/tmp/main_gui";
	char stepper_ifile_name[] = "/tmp/motorFeedback";
	char stepper_ofile_name[] = "/tmp/motorControl";
	char detection_ifile_name[]= "/tmp/noah_angle";

	simplePipe<double, 2> gui_simpleIn(gui_ifile_name, O_RDONLY);
	simplePipe<double, 2> gui_simpleOut(gui_ofile_name, O_WRONLY);
	simplePipe<double, 2> stepper_simpleIn(stepper_ifile_name, O_RDONLY);
	simplePipe<double, 2> stepper_simpleOut(stepper_ofile_name, O_WRONLY);
	simplePipe<double, 2> detection_simpleIn(detection_ifile_name, O_RDONLY);
	

	printf("\nDRONE DEMO START\n");

	while(true){
		// FIFO read
		int read_gui = gui_simpleIn.pipeIn((double (&)[2])gui_in);
		int read_stepper= stepper_simpleIn.pipeIn((double (&)[2])stepper_in);
		int read_detection = detection_simpleIn.pipeIn((double (&)[2])detection_in);


		// BATTERY VOLTAGE
		auto bat = autopilot_interface.current_messages.battery_status;
		gui_out.msg1 = (double)bat.voltages[0]/1000.0;

		// stepper current angle
		gui_out.msg2 = stepper_in.msg2;


		if(read_gui <=0 && read_stepper <= 0 && read_detection <=0){
			sleep_for(milliseconds(10));
			//continue;
		}
		else {
			message_handler(zylia, autopilot_interface, control_status, detection_in, gui_in, gui_out, stepper_in, stepper_out);
		}

		// FIFO write
		int write_gui = gui_simpleOut.pipeOut((double (&)[2])gui_out);
		int write_stepper= stepper_simpleOut.pipeOut((double (&)[2])stepper_out);
		//sleep_for(milliseconds(10));
		sleep_for(milliseconds(1000));
	}
	//   THREAD and PORT SHUTDOWN
	autopilot_interface.stop();
	port->stop();

	delete port;

	//   DONE
	return 0;

}

//	MESSAGE HANDLER
void message_handler(int zylia,
		Autopilot_Interface &api, 
		status &control_status,
		detection_message detection_in,
		gui_message gui_in, 
		gui_message &gui_out,
	 	stepper_message stepper_in,
	 	stepper_message &stepper_out)
{
	
	// handle gui message 
	switch((uint8_t)gui_in.msg1){
		case 0:
			break;
		case 1:
			play_tune(api, 0);
			initialize(control_status.init, 
					stepper_in.msg1, stepper_in.msg2, 
					stepper_out.msg1, stepper_out.msg2);
			break;
		case 2:
			play_tune(api, 2);
			if(control_status != 0){
				scan(zylia,
						control_status.scan,
						detection_in.detect,
						detection_in.detect_angle, 
						stepper_in.msg1, stepper_in.msg2,
						stepper_out.msg1, stepper_out.msg2);
			}
			break;
		case 3:
			play_tune(api, 2);
			motor_start(api, control_status.motors);
			break;
		case default:
			break;
	}
}

// INITIALIZE
void initialize( bool init, 
		double current_action, double current_angle, 
		double &desire_angle, double &sleep_time)
{
	// start initialize
	printf("INITIALIZING...\n");
	init = 0; //indicate need to reinitialize
	desire_angle = 361; //stepper initialize param
	sleep_time= -1; //stepper initialize param
	// return to the center
	if((current_action == 0) && (current_angle == 0)){
			desire_angle = 177;
			sleep_time = 0.0075;
			init = 1;
		}
	}
}

// DRONE SCAN
void scan(int zylia,
	 	double detect, double detect_angle, 
		double scan_in, double degree_in, 
		double current_action, double current_angle,
	 	double &scan_out, double &degree_out, 
		double &desire_angle, double &sleep_time)
{
	// start scan and listen for signal
	// if detect stop scan and turn to signal direction until some time
	// continue scanning
	if(scan_in == 1){
				scan_out = 1;
			if (detect == 1){
				int led_red[1] = {0x0000FF};
				ioctl(zylia, 0x00000205, led_red);
				//TODO
				double d_angle = std::fmod((current_angle + detect_angle), 360.0);
				printf("%f\n", d_angle);
				//desire_angle = d_angle; 
				return;
			}
			else{
				printf("SCAN START\n");
				int led_green[1] = {0xFF0000};
				ioctl(zylia, 0x00000205, led_green);
				// scan range
				double scan_min = 177 - degree_in/2;
				double scan_max = 177 + degree_in/2;
				// if stepper not busy go to angle
				if (current_action == 0 && current_angle != scan_min){
					desire_angle = scan_min;
				}
				else if (current_action == 0 && current_angle != scan_max){
					desire_angle = scan_max;
				}
			}
		}
	// Scan off
	else if(scan_in == 0){
		printf("SCAN STOP\n");
		int led_white[1] = {0xFFFFFF};
		ioctl(zylia, 0x00000205, led_white);
		scan_out = 0;
	}
}

// PLAY TUNE
void play_tune(Autopilot_Interface &api, double tune_id)
{
	// 0: button pressed
	// 1: initialize
	// 2: low battery
	char* tune;
	printf("PLAY TUNE %d\n", (int)tune_id);
	switch((uint8_t)tune_id){
		case 0:
			tune = "MFT200L16<cdefgab>cdefgab.c";
			api.play_tune(tune);
			break;
		case 1:
			tune = "MFL4<<<<A";
			for (int i=0;i<3;i++){
				api.play_tune(tune);
				this_thread::sleep_for(chrono::milliseconds(500));
			}
			break;
		case 2: 
			tune = "MFL32<<<G";
			api.play_tune(tune);
			break;
		default:
			break;
	}
}

//  MOTOR TEST 
void
motor_start(Autopilot_Interface &api, double motor_in, double &motor_out)
{
	// Legs up/down
	// Motor test On/Off
	if (motor_in == 1){
		api.landing_gear(true);
		for (int i = 0; i < 5; i++){
			api.motor_start(true, i);
			this_thread::sleep_for(chrono::milliseconds(10));
		}
		motor_out = 1;
	}
	else {
		api.landing_gear(false);
		for (int i = 0; i < 5; i++){
			api.motor_start(false,i);
			this_thread::sleep_for(chrono::milliseconds(10));
		}
		motor_out = 0;
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
		sleep_for(milliseconds(10));
	}

	int led_manual[1] = {1};
	int led_white[1] = {0xFFFFFF};
	int setmode = 0x00000203;
	int setcolor = 0x00000205;
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

