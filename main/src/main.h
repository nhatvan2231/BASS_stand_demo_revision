// ------------------------------------------------------------------------------
//   Includes
// ------------------------------------------------------------------------------

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

#include <common/mavlink.h>

#include "autopilot_interface.h"
#include "serial_port.h"
#include "udp_port.h"
#include "simplePipe.h"

struct sys_status{
	bool init = false;
	bool scan = false;
	bool motor = false;
	bool detect = false;
};

int main(int argc, char **argv);


int top(int argc, char **argv);

sys_status gui_message_handler(Autopilot_Interface &autopilot_interface, uint16_t gui_in, sys_status cur_status, uint8_t &cur_state);

void parse_commandline(int argc, char **argv, char *&uart_name, int &baudrate,
		bool &use_udp, char *&udp_ip, int &udp_port);

int zylia_setup(char zylia_path[]);

int main_control(Autopilot_Interface &autopilot_interface);

void motor_start(Autopilot_Interface &autopilot_interface, bool motors);

void play_tune(Autopilot_Interface &autopilot_interface, double msg);

bool signal_detection(int zylia, double detect_angle, double cur_angle, double &checkpoint, double (&stepper_out)[3]);

// quit handler
Autopilot_Interface *autopilot_interface_quit;
Generic_Port *port_quit;
void quit_handler( int sig );

