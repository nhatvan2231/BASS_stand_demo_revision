#!/bin/bash

echo "Relocating executable to bin"
mkdir -p $HOME/.local/bin
cp main/build/src/drone_demo $HOME/.local/bin
cp main/build/src/detection $HOME/.local/bin
cp FIFO_DOA/dir_of_arr $HOME/.local/bin
cp GPIO_stepper/stepper $HOME/.local/bin
cp python_gui/demo_gui.py $HOME/.local/bin
