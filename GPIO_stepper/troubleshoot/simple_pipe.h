#ifndef SIMPLE_PIPE_H
#define SIMPLE_PIPE_H
#include <fcntl.h> // O_RDONLY
#include <sys/stat.h> // mkfifo()
#include <unistd.h> // open()
#include <stdio.h> // fprintf();

// Simple FIFO class takes in FileName and Permissions
template <class T, size_t Size>
class simplePipe {
	public:
		simplePipe(char file_name[], int PERMISSIONS) {
			if(mkfifo(file_name, 0666) != 0) { // File already exists
				// Open file
				fifo = open(file_name, PERMISSIONS);
				struct stat st;

				// Check if the file is a FIFO
				if(fstat(fifo, &st) && !S_ISFIFO(st.st_mode)) {
					fprintf(stderr, "File exists but is not a named pipe.\n");
					close(fifo);
					return;
				}
			} else { // File successfully created
				// Open file
				fifo = open(file_name, PERMISSIONS);
			}
			if(PERMISSIONS == O_RDONLY || PERMISSIONS == O_WRONLY)
				perm = PERMISSIONS;
			else {
				fprintf(stderr, "Invalid permissions\nPlease choose O_RDONLY or O_WRONLY.\n");
				return;
			}
		}

		~simplePipe() {
			struct stat st;
			if(fstat(fifo, &st) && S_ISFIFO(st.st_mode))
				close(fifo);
		}

		int pipeIn(T (&message)[Size]) {
			if(perm == O_RDONLY)
				return read(fifo, message, Size*sizeof(T));
			else {
				fprintf(stderr, "Invalid permissions\nDid you mean O_RDONLY?\n");
				return -1;
			}
		}

		int pipeOut(T (&message)[Size]) {
			if(perm == O_WRONLY)
				return write(fifo, message, Size*sizeof(T));
			else {
				fprintf(stderr, "Invalid permissions\nDid you mean O_WRONLY?\n");
				return -1;
			}
		}
	private:
		int fifo; // FIFO file reference
		int perm; // Permissions: O_RDONLY or O_WRONLY
};
#endif
