#!/bin/bash
mkfifo /tmp/gui_main
mkfifo /tmp/main_gui
mkfifo /tmp/motorFeedback
mkfifo /tmp/motorControl
mkfifo /tmp/main_dection
mkfifo /tmp/noah_angle

$HOME/BASS_stand_demo_revision/GPIO_stepper/stepper &
wait
$HOME/BASS_stand_demo_revision/main/build/src/demo_main -d /dev/ttyACM0 &
wait
$HOME/BASS_stand_demo_revision/main/build/src/demo_detection &
wait
$HOME/BASS_stand_demo_revision/FIFO_DOA/dir_of_arr &

