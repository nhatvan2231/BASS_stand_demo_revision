#include <fcntl.h> // O_RDONLY O_WRONLY
#include "simplePipe.h" // simplePipe()
#include <string> // std::stod()
#include <iostream> // std::cerr std::cout std::endl
#include <iomanip> // std::setprecision()
#include <limits> // std::numeric_limits<double>::digits10
using namespace std;
#ifndef SIZE
#define SIZE 2 
#endif

int main(int argc, char* argv[]) {
	if(argc < 2) {
		cerr << "Usage: " << argv[0] << " <FIFO filename>";
		cerr << endl;
		return -1;
	}

	simplePipe<double, SIZE> test(argv[1], O_RDONLY);
	double message[SIZE];
	while(true) {
		int bytes = test.pipeIn((double (&)[SIZE])message);
		if(bytes > 0) {
			cout << "Message: ";
			for(int i = 0; i < SIZE; ++i)
				cout << setw(16) << message[i];
			cout << endl << "Bytes read: " << bytes << endl;
		}
		else{
			usleep(1000);
		}
	}
	return 0;
}

#undef SIZE
