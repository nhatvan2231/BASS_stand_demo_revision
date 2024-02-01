# bass_stand_demo
=======================
Python programming for BASS Project 2021

This is the GUI for the drone stand and the command to run the programs on startup


# Videostream on secondary monitor
## Dependencies
<ul>
  <li>Raspbian installation (with graphical environment)</li>
  <li>mpv</li>
</ul>

## Command
========================
    mpv --untimed --profile=low-latency --speed=100.0 --video-latency-hacks=yes --framedrop=no --opengl-glfinish=yes --opengl-swapinterval=0 --no-audio --stop-screensaver --fs --panscan=1.0 --pause=no /dev/video0

Note: Add this line to ~/.xsessionrc to have it start on login


C-UART Interface Example
========================

This is a simple MAVLink to UART interface example for *nix systems that can allow communication between Pixhawk and an offboard computer.

This example will receive one MAVLink message and send one MAVLink message.


Building
========

```
$ cd c_uart_interface_example/
$ make
```

Hardware Setup
=========

Connect the USB programming cable to your Pixhawk.  

If you want to be able to interact with this example in Pixhawk's NuttX shell, you'll need a Telemetry Radio or an FTDI developer's cable.  See the Exploration section below for more detail.

Also Note: Using a UART (serial) connection should be preferred over using the USB port for flying systems.  The reason being that the driver for the USB port is much more complicated, so the UART is a much more trusted port for flight-critical functions.  To learn how this works though the USB port will be fine and instructive.

Execution
=========

You have to pick a port name, try searching for it with 
```

$ ls /dev/ttyACM* 
$ ls /dev/ttyUSB*
```

Alternatively, plug in Pixhawk USB cable again and issue the command:
```
$ dmesg
```
The device described at the bottom of dmesg's output will be the port on which the Pixhawk is mounted. 

The Pixhawk USB port will show up on a `ttyACM*`, an FTDI cable will show up on a `ttyUSB*`.


Run the example executable on the host shell:

```
$ cd c_uart_interface_example/
$ ./mavlink_control -d /dev/ttyACM0
```

To stop the program, use the key sequence `Ctrl-C`.

Exploration
===========

There are a few things to explore past this example.

First you can connect via:
* a [Telemetry Radio](https://docs.px4.io/en/telemetry/) on TELEM1 or 2
* an [FTDI cable](https://www.sparkfun.com/products/9718) on TELEM2 or Serial 4

> **Note** 
> * Serial 5 can't be used to receive data without reconfiguration (its receive pin is occupied by a second NuttX shell).
> * TELEM1 is typically dedicated to Telemetry Radio, but if you're using another port you will need to make sure it is not configured for use by another peripheral.
> * If using FTDI with a TELEM port, connect all the pins to corresponding pins on port.
> * If using FTDI with SERIAL4 connect the TX, RX GND and 5V pins (CTS, RTS need not be connected).


With this you'll be able to start a second port for communication, and leave the USB port available for viewing prints in the NuttX shell.  

For steps 2 and 3 from the above tutorial, you'll use a different port.  On the off-board computer side, the port might now be `/dev/ttyUSB0`.  On the Pixhawk side, here the port mappings are in the table below.

| PX4 UART | NuttX UART |
|----------|------------|
| Telem 1  | /dev/ttyS1 |
| Telem 2  | /dev/ttyS2 |
| Serial 4 | /dev/ttyS6 |

Now add a print statement in the Pixhawk Firmware to see received messages.  Build and upload this to Pixhawk.

```
[Firmware/src/modules/mavlink/mavlink_receiver.cpp]
/* if read failed, this loop won't execute */
for (ssize_t i = 0; i < nread; i++) {
	if (mavlink_parse_char(_mavlink->get_channel(), buf[i], &msg, &status)) {

		/* --- REPORT HANDLING OF MESSAGE --- */
		printf("\n");
		printf("HANDLE MESSAGE\n");
		printf("MSGID:%i\n",msg.msgid);

		/* handle generic messages and commands */
		handle_message(&msg);
```

Open the system terminal as described here: https://dev.px4.io/en/debug/system_console.html 

On the off-board side, in another terminal run the `c_uart_interface_example` executable. You should see output in the NuttX shell similar to this:

```
HANDLE MESSAGE
MSGID:76

HANDLE MESSAGE
MSGID:84

(...)

HANDLE MESSAGE
MSGID:84

HANDLE MESSAGE
MSGID:76
```

Past this, you can:
- Modify the received message data type
- Modify the sent message data type

Simulation
===========

There is also the possibility to connect this example to the simulator using:

```
$ ./mavlink_control -u 127.0.0.1 -a
```
The -a argument enables arming, takeoff and landing of the copter. Use this argument with care on a real copter!
