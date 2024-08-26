#include <fcntl.h> // O_RDONLY O_WRONLY
#include "simple_pipe.h" // SimplePipe()
#include <string> // std::stod()
#define SIZE 4

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;

	simplePipe<double, SIZE> test(argv[1], O_RDONLY);
	double message[SIZE];
	while(true) {
		int bytes = test.pipeIn((double (&)[SIZE])message);
		if(bytes > 0) {
			printf("Message: ");
			for(int i = 0; i < SIZE; ++i)
				printf("%f\t", message[i]);
			printf("\nBytes read: %d\n", bytes); fflush(stdout);
		}
		else{
			usleep(1000);
		}
	}
	return 0;
}

#undef SIZE
