#include "detection.h"

int main(){

        char detec_ifile_name[] = "/tmp/main_detection";
	//char detec_ofile_name[] = "/tmp/detection_main";

	char noah_ifile_name[] = "/tmp/noah_angle";

        char stepper_ifile_name[] = "/tmp/motorFeedback";
        char stepper_ofile_name[] = "/tmp/motorControl";

	double stepper_in[2];
        double stepper_out[3];
	double noah_in[2];
	bool detec_in;
	bool detec_out;

        simplePipe<double, 2> noah_simpleIn(noah_ifile_name, O_RDONLY);

        simplePipe<double, 2> stepper_simpleIn(stepper_ifile_name, O_RDONLY);
        simplePipe<double, 3> stepper_simpleOut(stepper_ofile_name, O_WRONLY);

        simplePipe<bool, 1> detec_simpleIn(detec_ifile_name, O_RDONLY);
        //simplePipe<bool, 1> detec_simpleOut(detec_ofile_name, O_WRONLY);


	int read_bytes = 0;
	int write_bytes = 0;

	char zylia_path[] = "/dev/zylia-zm-1_0";
	const int setmode = 0x00000203;
	const int setcolor = 0x00000205;
	const int led_manual[1] = {1};
	const int led_red[1] = {0x0000FF};
	const int led_yellow[1] = {0x00FFFF};
	int fd = open(zylia_path, O_RDWR | O_NONBLOCK);
	ioctl(fd, setmode, led_manual);



	double d_angle;
	while(true){
		detec_simpleIn.pipeIn((bool (&)[1])detec_in);
		if (!detec_in){
			printf("NOT READY\n");
		       	usleep(1000);
			double *tmp;
			noah_simpleIn.pipeIn((double (&)[2])tmp); // clear the buffer if any detected
		       	continue;
		}

		read_bytes = noah_simpleIn.pipeIn((double (&)[2])noah_in);
		if (read_bytes > 0){
			if(noah_in[0] == -42069) {
				// Save position
				read_bytes = stepper_simpleIn.pipeIn((double (&)[2])stepper_in);
				while(read_bytes <=0){
					read_bytes = stepper_simpleIn.pipeIn((double (&)[2])stepper_in);
				}
				ioctl(fd, setcolor, led_yellow);
				printf("Current Angle: %f\n", stepper_in[0]);
				continue;
			}
			d_angle = std::fmod((stepper_in[0] + (360 + noah_in[0])), 360.0);
			printf("RECEIVED -> Angle: %f\n", d_angle);
			stepper_out[0] = d_angle;
			stepper_out[1] = 0.0025;
			stepper_out[2] = 5;
			detec_out = true;
			ioctl(fd, setcolor, led_red);

                        write_bytes= stepper_simpleOut.pipeOut((double (&)[3])stepper_out);
			while(write_bytes <=0){
                        	write_bytes = stepper_simpleOut.pipeOut((double (&)[3])stepper_out);
			}
                        //write_bytes= detec_simpleOut.pipeOut((bool (&)[1])detec_out);
			//while(write_bytes <= 0){
                        //	write_bytes = detec_simpleOut.pipeOut((bool (&)[1])detec_out);
			//}
		}
	}
}







		



