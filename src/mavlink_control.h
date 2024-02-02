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
 * @file mavlink_control.h
 *
 * @brief An example offboard control process via mavlink, definition
 *
 * This process connects an external MAVLink UART device to send an receive data
 *
 * @author Trent Lukaczyk, <aerialhedgehog@gmail.com>
 * @author Jaycee Lock,    <jaycee.lock@gmail.com>
 * @author Lorenz Meier,   <lm@inf.ethz.ch>
 *
 */


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

struct status;
struct message;

int main(int argc, char **argv);
int top(int argc, char **argv);

void message_handler(int fd, 
										Autopilot_Interface &autopilot_interface, 
										status &control_status,
										detection_message detection_in,
									 	gui_message gui_in,
	 									gui_message &gui_out,
									 	stepper_message stepper_in,
									 	stepper_message &stepper_out);

void parse_commandline(int argc, char **argv, char *&uart_name, int &baudrate,
		bool &use_udp, char *&udp_ip, int &udp_port);
int zylia_setup(char zylia_path[]);

// ------------------------------------------------------------------------------
// Messages
// ------------------------------------------------------------------------------
// param ->  initialize status, current action, current angle,  desire angle, sleep time
void initialize(bool &init, double msg_in1, double msg_in2, double &msg_out1, double &msg_out2);

// param -> zylia, detect, detect_angle, scan in, scan degree in, current action, current angle, scan out,
// 					 scan degree out desire angle, sleep time
void scan(int fd,
		bool &scan,
	 	double msg_in1, double msg_in2, 
		double msg_in3, double msg_in4, 
		double msg_in5, double msg_in6,
	 	double &msg_out1, double &msg_out2, 
		double &msg_out3, double &msg_out4);

void motor_start(Autopilot_Interface &autopilot_interface, bool &motor, double msg_in, double &msg_out);

void play_tune(Autopilot_Interface &autopilot_interface, double msg);

// quit handler
Autopilot_Interface *autopilot_interface_quit;
Generic_Port *port_quit;
void quit_handler( int sig );

