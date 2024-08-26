CPP=g++
CPPFLAGS=-std=c++17
LDFLAGS=-lgsl -lgslcblas -lfftw3 -lpthread -lasound -lm
SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%.o)

IIR_SRC=$(wildcard iir/*.cpp)
IIR_OBJ=$(IIR_SRC:iir/%.cpp=iir/%.o)

dir_of_arr: $(OBJ) $(IIR_OBJ)
	$(CPP) $(CPPFLAGS) $(OBJ) $(IIR_OBJ) -o $@ $(LDFLAGS) 

%.o : %.cpp %.h
	$(CPP) $(CPPFLAGS) -c $< -o $@

iir/%.o : iir/%.cpp iir/%.h
	$(CPP) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf dir_of_arr *.o iir/*.o
