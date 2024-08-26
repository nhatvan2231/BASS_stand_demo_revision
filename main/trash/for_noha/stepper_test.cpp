#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <cmath>
#include <string.h>
#include <inttypes.h>
#include <fstream>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <chrono>
#include <thread>

using std::string;
using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

#include "simplePipe.h"

struct message{
	double msg1;
	double msg2;
	double msg3;
};

struct sys_status{
	bool init = false;
	bool scan = false;
	bool motor = false;
};
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

sys_status gui_message_handler(uint16_t gui_in, sys_status cur_status, uint8_t &cur_state){
	sys_status status = cur_status;
	switch(gui_in){
		case 1:
			//play_tune(autopilot_interface, 0);
			cur_state = 0;
			status.init = true;
			status.scan = false;
			status.motor = false;
			break;
		case 2:
			if (cur_state < 2) break;
			//play_tune(autopilot_interface, 1);
			status.scan = !status.scan;
			if(status.scan == false) cur_state = 1;
			printf("SCAN: %d\n", status.scan);
			break;
		case 3:
			if (cur_state < 2) break;
			//play_tune(autopilot_interface, 1);
			status.motor = !status.motor;
			//motor_start(autopilot_interface, status.motor);
			break;
		default:
			break;
	}
	return status;
}

int main(){
	char zylia_path[] = "/dev/zylia-zm-1_0";
	int zylia = zylia_setup(zylia_path);

	/* START NAMED PIPES
	 * Named pipe between gui and main
	 * Named pipe between stepper motor and main
	 */

	uint8_t cur_state = 0;
	uint8_t next_state = 0;
	bool write = false;
	sys_status status;

	uint16_t gui_in; // Initialzie, scan, motor
	double gui_out[2]; // battery, current angle
	double stepper_in[2]; // current angle, current action
	double stepper_out[3]; // desire angle, sleep time, hold time
	bool detec_in = false; // flag
	bool detec_out = true; // flag

	char gui_ifile_name[] = "/tmp/gui_main";
	char gui_ofile_name[] = "/tmp/main_gui";
	char stepper_ifile_name[] = "/tmp/motorFeedback";
	char stepper_ofile_name[] = "/tmp/motorControl";
	char detection_ifile_name[] = "/tmp/detection_main";
	char detection_ofile_name[] = "/tmp/main_detection";

	simplePipe<uint16_t, 1> gui_simpleIn(gui_ifile_name, O_RDONLY);
	simplePipe<double, 2> gui_simpleOut(gui_ofile_name, O_WRONLY);
	simplePipe<double, 2> stepper_simpleIn(stepper_ifile_name, O_RDONLY);
	simplePipe<double, 3> stepper_simpleOut(stepper_ofile_name, O_WRONLY);

	simplePipe<bool, 1> detection_simpleIn(detection_ifile_name, O_RDONLY);
	simplePipe<bool, 1> detection_simpleOut(detection_ofile_name, O_WRONLY);

	// zylia LED
	int led_red[1] = {0x0000FF};
	int led_green[1] = {0x00FF00};
	int led_white[1] = {0xFFFFFF};
	

	printf("\nDRONE DEMO START\n");

	while(true){
		// FIFO read
		//int read_stepper = stepper_simpleIn.pipeIn((double (&)[2])stepper_in);
		//double tmpReadIn[2] = {0,0};


		while(true){
			int read_stepper = stepper_simpleIn.pipeIn((double (&)[2])stepper_in);
			int read_gui = gui_simpleIn.pipeIn((uint16_t (&)[1])gui_in);
			int read_detection = detection_simpleIn.pipeIn((bool (&)[1])detec_in);

			//printf("Current angle: %f Current action: %f\n", stepper_in[0], stepper_in[1]);
			//if(detec_in){// && cur_state > 1){
			//	ioctl(zylia, 0x00000205, led_red);
			//       	printf("DETECT: %d\n", detec_in);
			//       	continue;
			//}

			if(read_gui > 0){
				//printf("Out: %d %d %d\n", status.init, status.scan, status.motor);
				status = gui_message_handler(gui_in, status, cur_state);
				break;
			}
			if(read_stepper > 0 && stepper_in[1] <= 0) {
				printf("Goal reach\n");
				cur_state = next_state;
				break;
			} 

			//if(read_stepper > 0 && tmpReadIn[1] <= 0) {
			//	printf("Received -> Bytes: %d Angle: %f Action: %f\n", read_stepper, tmpReadIn[0], tmpReadIn[1]);
			//	stepper_in[0] = tmpReadIn[0];
			//	stepper_in[1] = tmpReadIn[1];
			//	printf("Goal reach\n");
			//	cur_state = next_state;
			//	break;
			//} 
			usleep(1000);
		}


		//printf("Current angle: %f Current action: %f\n", stepper_in[0], stepper_in[1]);

		switch(cur_state){
			case 0:
				if (!status.init) break;
				printf("INITIALIZING...\n");
				ioctl(zylia, 0x00000205, led_white);
				stepper_out[0] = 361;
				stepper_out[1] = -1;
				stepper_out[2] = 0;
				detec_out = false;
				write = true;
				//motor_start(autopilot_interface, status.motor);
				//printf("Transition 0 to 1\n");
				next_state = 1;
				break;
			case 1:
				printf("GOTO MIDDLE\n");
				stepper_out[0] = 177;
				stepper_out[1] = 0.0075;
				stepper_out[2] = 1;
				detec_out = false;
				status.init = false;
				write = true;
				//printf("Transition 1 to 2\n");
				next_state = 2;
				break;
			case 2:
				if (!status.scan) break;
				printf("GOTO RIGHT\n");
				ioctl(zylia, 0x00000205, led_green);
				stepper_out[0] = 177-45;
				stepper_out[1] = 0.0075;
				stepper_out[2] = 1;
				detec_out = true;
				write = true;
				next_state = 3;
				break;
			case 3:
				if (!status.scan) break;
				printf("GOTO LEFT\n");
				ioctl(zylia, 0x00000205, led_green);
				stepper_out[0] = 177 + 45;
				stepper_out[1] = 0.0075;
				stepper_out[2] = 1;
				detec_out = true;
				write = true;
				next_state = 2;
				break;
			default:
				break;
		}

		// FIFO write
		if(write){
			int write_bytes = stepper_simpleOut.pipeOut((double (&)[3])stepper_out);
			while(write_bytes <= 0){
				write_bytes = stepper_simpleOut.pipeOut((double (&)[3])stepper_out);
				printf("Reattempting to write\n");
			}

			write_bytes = detection_simpleOut.pipeOut((bool (&)[1])detec_out); 
			while(write_bytes <= 0){
				write_bytes = detection_simpleOut.pipeOut((bool (&)[1])detec_out); 
				printf("Reattempting to write\n");
			}
			
			write = false;
			//printf("Writing: %f %f %f\n", stepper_out[0], stepper_out[1], stepper_out[2]);
		}
		usleep(1000);
	}

}
