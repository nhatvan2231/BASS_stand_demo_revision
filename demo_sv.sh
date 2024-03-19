#!/bin/bash
mkfifo /tmp/gui_main
mkfifo /tmp/main_gui
mkfifo /tmp/motorFeedback
mkfifo /tmp/motorControl
mkfifo /tmp/main_dection
mkfifo /tmp/noah_angle

#$HOME/BASS_stand_demo_revision/GPIO_stepper/stepper &
#$HOME/BASS_stand_demo_revision/main/build/src/demo_main -d /dev/ttyACM0 &
#$HOME/BASS_stand_demo_revision/main/build/src/demo_detection &
#$HOME/BASS_stand_demo_revision/FIFO_DOA/dir_of_arr &
stepper &
demo_main -d /dev/ttyACM0 &
#demo_detection &
dir_of_arr &
	# Check the filenames and make sure they match in main
	# wherever you call from (you call from home)
	# Ex: char[] arrayGeometry = "/home/ncpa/..."
