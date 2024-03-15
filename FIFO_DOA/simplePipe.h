#ifndef SIMPLE_PIPE_H
#define SIMPLE_PIPE_H
#include <fcntl.h> // O_RDONLY O_WRONLY O_NONBLOCK
#include <sys/stat.h> // mkfifo()
#include <unistd.h> // open()
#include <stdio.h> // fprintf();
#include <string> // std::string
// Simple FIFO class takes in FileName and Permissions
template <class T, size_t Size>
class simplePipe {
	public:
		simplePipe(std::string filename, int PERMISSIONS) {
			if(PERMISSIONS == O_RDONLY || PERMISSIONS == O_WRONLY)
				perm = PERMISSIONS;
			else {
				fprintf(stderr, "Invalid permissions\nPlease choose O_RDONLY or O_WRONLY.\n");
				return;
			}

			fifo_filename = filename;
			if(mkfifo(fifo_filename.c_str(), 0666) != 0) { // File already exists
				// Open file
				fifo = open(fifo_filename.c_str(), PERMISSIONS | O_NONBLOCK);
				struct stat st;

				// Check if the file is a FIFO
				if(fstat(fifo, &st) && !S_ISFIFO(st.st_mode)) {
					fprintf(stderr, "File exists but is not a named pipe.\n");
					close(fifo);
					return;
				}
			} else { // File successfully created
				// Open file
				fifo = open(fifo_filename.c_str(), PERMISSIONS | O_NONBLOCK);
			}
		}

		~simplePipe() {
			struct stat st;
			if(fstat(fifo, &st) && S_ISFIFO(st.st_mode))
				close(fifo);
		}

		int pipeIn(T (&message)[Size]) {
			if(perm == O_RDONLY) {
				return read(fifo, message, Size*sizeof(T));
			}
			else {
				fprintf(stderr, "Invalid permissions\nDid you mean O_RDONLY?\n");
				return -1;
			}
		}

		int pipeOut(T (&message)[Size]) {
			if(fifo < 0) {
				fprintf(stderr, "Attempting to re-open the fifo file.\n");
				fifo = open(fifo_filename.c_str(), perm | O_NONBLOCK);

				struct stat st;
				// Check if the file is a FIFO
				if(fstat(fifo, &st) && !S_ISFIFO(st.st_mode)) {
					fprintf(stderr, "File exists but is not a named pipe.\nAre you reading from the FIFO file?\n");
					close(fifo);
					return -1;
				}
			}

			if(perm == O_WRONLY)
				return write(fifo, message, Size*sizeof(T));
			else {
				fprintf(stderr, "Invalid permissions\nDid you mean O_WRONLY?\n");
				return -1;
			}
		}
	private:
		std::string fifo_filename; // FIFO file name
		int fifo; // FIFO file reference
		int perm; // Permissions: O_RDONLY or O_WRONLY
};
#endif
