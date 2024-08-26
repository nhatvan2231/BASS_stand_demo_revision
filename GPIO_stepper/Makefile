CXX := g++
CXXFLAGS := -DFIFO_OUTPUT --std=c++17 -lpigpiod_if2 -lrt
#CXXFLAGS := --std=c++17 -lpigpiod_if2 -lrt

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o, $(SOURCES))

# ADD MORE WARNINGS!
#WARNING := -Wall -Wextra

# .PHONY means these rules get executed even if
# files of those names exist.
.PHONY: all clean

# The first rule is the default, ie. "make",
# "make all" and "make run" mean the same
all: stepper

clean:
	$(RM) $(OBJECTS) stepper

# Linking the executable from the object files
stepper: $(OBJECTS)
	$(CXX) $(WARNING) $^ -o $@ $(CXXFLAGS) 

%.o: %.cpp %.h
	$(CXX) $(WARNING) $(CXXFLAGS) -c $< -o $@
