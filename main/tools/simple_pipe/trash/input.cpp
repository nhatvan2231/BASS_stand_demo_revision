#include <fcntl.h> // O_RDONLY O_WRONLY
#include "simple_pipe.h" // SimplePipe()
#include <string> // std::stod()
#include <unistd.h> // usleep(microseconds)
#define SIZE 4

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;

	simplePipe<double, SIZE> test(argv[1], O_WRONLY);
	double message[SIZE];
	for(int i = 0; i < SIZE; ++i)
		message[i] = std::stod(argv[i+2]);

	while(true) {
		int writBytes = test.pipeOut((double (&)[SIZE])message);
		printf("Bytes Written: %d\n", writBytes);
		usleep(1000);
	}
	return 0;
}
#undef SIZE
