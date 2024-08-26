# Dependencies
g++

# Usage
Copy **`simple_pipe.h`** to your working directory

# Examples
The makefile produces two executables: **`bin/input`** and **`bin/output`**
```
make examples
```
## Usage
To insert data into a FIFO file
```
bin/input <FIFO_FILENAME> <data[0]> <data[1]> <data[2]> <data[3]>
```
To display insert data from a FIFO file
```
bin/output <FIFO_FILENAME>
```
