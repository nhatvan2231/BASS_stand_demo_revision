Main Interface
========
This is the main interface for the BASS stand demo. The objective is a demo of a drone hovering and listenning for a high frequency impulse noise (a clap or gun shot). This interface includes communication between an offboard computer, Pixhawk, stepper motor, zylia microphone's LED, signal detection and python GUI. The offboard computer controls the other interface using named pipe communication.
- Pixhawk -> start autopilot, controlling buzzer and controlling ESC motors
- Stepper motor -> control initialization and scanning scheme
- Zylia -> control LED control scheme
- GUI -> receiving messages to control initialization, scanning and ESC motors
- Detection -> handle detection interruption

Building
========
Initial build
```
$ rm -rf build
$ mkdir build
$ cd build
$ cmake ..
$ make
```	
After
```
$ cmake --build build
```

Execution
=========
```
$ ./build/src/drone_demo
$ ./build/src/detection
```


References
=========
[Mavlink c_uart_interface_example](https://github.com/mavlink/c_uart_interface_example)

