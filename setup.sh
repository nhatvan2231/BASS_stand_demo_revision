#!/bin/bash

echo "Relocating executable to bin"
mkdir -p $HOME/.local/bin

cp demo_sv.sh $HOME/.local/bin
cp main/build/src/demo_main $HOME/.local/bin
#cp main/build/src/demo_detection $HOME/.local/bin
cp python_gui/demo_gui.py $HOME/.local/bin
cp FIFO_DOA/dir_of_arr $HOME/.local/bin
cp GPIO_stepper/stepper $HOME/.local/bin


