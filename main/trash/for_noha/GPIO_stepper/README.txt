Dependencies:
pigpiod

	Installing pigpiod:
	```
	wget https://github.com/joan2937/pigpio/archive/master.zip
	unzip master.zip
	cd pigpio-master
	make
	sudo make install
	```
sudo ln -s $PWD/pigpiod.service /lib/systemd/system/
sudo systemctl start pigpiod

Compiling executable:

```
make
```

Runtime:
```
./stepper
```

Usage:
stepper requires that its FIFO output be continuously read to function normally.

	stepper FIFO output gives two data points:
		A) action message
			0) No action
			1) Initialization
			2) Moving to angle
		B) current angle (in degrees)
			[0 to 360)
	stepper FIFO input takes two doubles:
		A) desired angle (degrees)
		B) time to sleep between steps (seconds)

	The following ranges of values command the stepper to perform that action
		Initialization:
			A) >= 360
			B) < 0
		Go to angle:
			A) Ranges from [0 to 360)
			B) Ranges from [0 to READ_PERIOD)
				Note: typically pick a number that is reasonably low (0.01 seconds)
		Exit:
			A) < 0
			B) < 0
