/*
	This program demonstrates the ability to control the LED on the Zylia using the kernel driver's ioctl interface.
	compile: gcc -o run main.c
	run: ./run
*/

#include <signal.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>

#define Z_IOCTL_DEVICE						"/dev/zylia-zm-1_0"
#define Z_IOCTL_S_LED_BRIGHTNESS			0x00000201
#define Z_IOCTL_S_LED_CONTROL_MODE		0x00000203
#define Z_IOCTL_S_LED_COLOR				0x00000205

// led color array
uint8_t led_color[3] = { 0, 255, 0 };

// device file descriptor
int z_dev;

// run flag
volatile bool run = true;

// signal handler function
void sig_handler(int signo)
{
	run = false;
	if (z_dev) {
		// reset the control mode to auto
		uint8_t ctrl_mode = 0;
		int ret = ioctl(z_dev, Z_IOCTL_S_LED_CONTROL_MODE, &ctrl_mode);
		if (ret < 0) {
			printf("Set control mode failed: %d\n", ret);
		}

		// close the device
		close(z_dev);
	}

	printf("\nDone\n");
}


int main(int argc, char **argv)
{
	int ret;

	// Set sig_handler function as signal handler for the SIGINT signal:
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		printf("An error occurred while setting the signal handler.\n");
		return 1;
	}

	printf("To end this demo, hit ctrl+c\n");

	// open the device
	z_dev = open(Z_IOCTL_DEVICE, 0);
	if (z_dev < 0) {
		printf("Cannot open %s\n", Z_IOCTL_DEVICE);
		return z_dev;
	}

	// set the control mode to manual
	uint8_t ctrl_mode = 1;
	ret = ioctl(z_dev, Z_IOCTL_S_LED_CONTROL_MODE, &ctrl_mode);
	if (ret < 0) {
		printf("Set control mode failed: %d\n", ret);
		return ret;
	}

	// set the brightness to 100%
	uint8_t brightness = 255;
	ret = ioctl(z_dev, Z_IOCTL_S_LED_BRIGHTNESS, &brightness);
	if (ret < 0) {
		printf("Set brightness failed: %d\n", ret);
		return ret;
	}

	// loop
	while (run) {
		// this algorithm taken from https://codepen.io/Codepixl/pen/ogWWaK
		// change the color
		if (led_color[0] > 0 && led_color[2] == 0) {
			led_color[0]--;
			led_color[1]++;
		}
		if (led_color[1] > 0 && led_color[0] == 0) {
			led_color[1]--;
			led_color[2]++;
		}
		if (led_color[2] > 0 && led_color[1] == 0) {
			led_color[2]--;
			led_color[0]++;
		}

		// send the new color
		ret = ioctl(z_dev, Z_IOCTL_S_LED_COLOR, &led_color);
		if (ret < 0) {
			printf("Set led color failed: %d\n", ret);
			return ret;
		}

		// wait
		usleep(20000);
	}

	return 0;
}
