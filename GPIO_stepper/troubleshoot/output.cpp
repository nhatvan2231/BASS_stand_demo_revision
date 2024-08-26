#include <fcntl.h> // O_RDONLY O_WRONLY
#include "simple_pipe.h" // SimplePipe()
#include <string> // std::stod()

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;

	simplePipe<double, 2> test(argv[1], O_RDONLY);
	double message[2];
	while(true) {
		int bytes = test.pipeIn(message);
		if(bytes > 0)
			printf("Message: %f, %f\nBytes: %d\n", message[0], message[1], bytes);
	}
	return 0;
}
