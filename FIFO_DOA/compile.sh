#!/bin/bash

g++ -c *.cpp -std=c++17
g++ -o run *.o iir/*.o -lgsl -lgslcblas -lfftw3 -lpthread -lasound
