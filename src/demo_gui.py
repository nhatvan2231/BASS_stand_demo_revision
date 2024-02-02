#!/bin/df/env python                         
import tkinter as tk
from tkinter import * 
#import RPi.GPIO as GPIO 
import time
import threading
import os
import fcntl
import numpy as np
import struct
#from socket import *
#from control_test import *
import tkinter.messagebox

NCPA_LOGO_FILE="../pic/bass_stand_logo.png"

# Messages -> [<id>] [<value>]
# --Messages Out
# [0] Tune
# [1] Initialize Purple
# [2] Scan Green
# [3] Motor Yellow

# --Messages In/
# [0] Battery
# [1] Current angle

MSG_IN_SIZE = 2
MSG_OUT_SIZE = 1
FIFO_IN = '/tmp/main_gui'
FIFO_OUT = '/tmp/gui_main'

messageIn = np.zeros((MSG_IN_SIZE,), dtype=np.float64)
messageOut = np.zeros((MSG_OUT_SIZE,), dtype=np.float64)

# read FIFO messages
def simplePipeIn(FIFO, SIZE = 2):
    global messageIn
    with open(FIFO, 'rb', os.O_NONBLOCK) as pipe:
        while True:
            data = pipe.read(SIZE * np.dtype(np.float64).itemsize) # number of messages * datatype = number of bytes to read
            if len(data) == 0:
                time.sleep(0.001)
                continue
            messageIn = struct.unpack('{0}d'.format(SIZE),data)
            time.sleep(0.001)

# write FIFO messages
def simplePipeOut(FIFO, SIZE = 2):
    with open(FIFO, 'wb', os.O_NONBLOCK) as pipe:
        print(messageOut)
        data = struct.pack('{0}d'.format(SIZE), *messageOut)
        pipe.write(data)
    time.sleep(0.001)

def purple_button():   # Initialize Button
    # TODO
    play_tune(1)
    messageOut[0] = 1;
    simplePipeOut(FIFO_OUT)

def green_button(): # Scan
    # TODO
    play_tune(3)
    messageOut[0] = 2
    simplePipeOut(FIFO_OUT)

def yellow_button(): # Motor Test
    # TODO
    play_tune(3)
    messageOut[0] = 3

def play_tune(tune):
    pass
    # Tune:
    # 0: button press
    # 1: initialize
    # 2: low battery
    messageOut[2] = tune;
    simplePipeOut(FIFO_OUT)

def battery_callback(): # Battery status
    # TODO
    while(True):
        #print(messageIn[0])
        batt_voltage = messageIn[0]
        label_battery_status_update.config(text = "{0} V".format(batt_voltage), font = ("Times", 16,'bold'), relief= FLAT, fg = 'red', bg = '#FFFFFF')
        if batt_voltage <= 20.0:
            messageOut[3] = 0
            #simplePipeOut(FIFO_OUT)
            #low_battery_voltage()
        time.sleep(1)


#pop up warning for low battery
def low_battery_voltage(): 
    play_tune(2)
    warning = Toplevel(window, bg = 'red')
    warning.title('WARNING!')
    warning.wm_attributes('-fullscreen',"True")
    warning.geometry('800x480')
    canvas = tkinter.Canvas(warning, bg = 'black', width = 800, height = 480)
    canvas.create_polygon(-100, 0, 0, 0, -100, 480, -200, 480, fill = 'yellow', tags = 'polygon')
    canvas.create_polygon(100, 0, 200, 0, 100, 480, 0, 480, fill = 'yellow', tags = 'polygon')
    canvas.create_polygon(300, 0, 400, 0, 300, 480, 200, 480, fill = 'yellow', tags = 'polygon')
    canvas.create_polygon(500, 0, 600, 0, 500, 480, 400, 480, fill = 'yellow', tags = 'polygon')
    canvas.create_rectangle(0,120,750,280, fill = 'yellow')
    canvas.create_text(300, 200, text = 'BATTERY LOW!\nUNPLUG BATTERY', justify = tk.CENTER,  fill='red', font = ('times', 42, 'bold'))
    canvas.pack()
    GPIO.cleanup()
    def first():
        canvas.configure(bg = 'yellow')
        canvas.itemconfig('polygon', fill = 'black')
        warning.after(300, second)
    def second():
        canvas.configure(bg = 'black')
        canvas.itemconfig('polygon', fill = 'yellow')
        warning.after(300, first)
    first()
    warning.mainloop()


# Setting up GUI Window
window = tk.Tk()
window.title("BOX-DRONE INTERFACE")
window.geometry("800x480")
frame = Frame(window, bg='#FFFFFF', borderwidth=0)
frame.pack(expand=1, fill = BOTH)

#NCPA Image
logo = PhotoImage(file = NCPA_LOGO_FILE)
label_logo = Label(frame, image=logo, font = ("Times", 18), relief = 'flat',borderwidth = 0)
label_logo.grid(row=0, column=0, columnspan = 3)

#Displaying Battery Status on GUI
label_battery = Label(frame, text = 'BATTERY STATUS: ', font = ("Times", 16,'bold'), relief= FLAT, fg = 'red', bg = '#FFFFFF')
label_battery.grid(row=6, column=0, columnspan = 1,sticky ="SW", pady = 8)        
label_battery_status_update = Label(frame, text = ' ', font = ("Times", 19,'bold'), relief= FLAT, bg = '#FFFFFF')
label_battery_status_update.grid(row=6, column=1, columnspan =2, sticky = "SEWE", pady = 8)

#Displaying Current angle 
current_angle = Label(frame, text = "{0}{1}".format(messageIn[1],u"\u00b0"), font = ("Times", 30),width = 10, relief= RIDGE, anchor = tk.CENTER, bg = '#00FF00')
current_angle.grid(row=1,column=1, pady= 10)

#Constructing / Customizing Buttons
butt_grow = Button(frame, text = '   + DEGREES   ',  font = (16), command = lambda: red_button(True), activebackground = "red", bg = "red", height = 3)
butt_grow.grid(row=4, column = 2, pady=5, padx = 10)
butt_shrink = Button(frame, text = '   - DEGREES   ', command = lambda: red_button(False), font = (16), activebackground = "red", bg = "red", height = 3)
butt_shrink.grid(row=4, column = 0, pady=5, padx = 10)
green = Button(frame, text = '     ON/OFF     ', command =  green_button, font = (16), activebackground = "green", bg = "green", height = 3)
green.grid(row=5, column = 0, pady=5, padx = 10)
yellow = Button(frame, text = 'HOVER ON/OFF', command = yellow_button, font = (16), activebackground = "yellow",  bg = "yellow", height = 3) 
yellow.grid(row=5, column = 1, pady=5, padx = 10)
purple = Button(frame, text = '    INITIALIZE    ', command = purple_button, font = (16), activebackground = 'magenta', bg = 'magenta', height = 3)
purple.grid(row=5, column = 2, pady=5, padx = 10)

#pipeOut = threading.Thread(target=simplePipeOut, args=(FIFO_OUT,))
#pipeOut.start()

battery_thread = threading.Thread(target=battery_callback)
battery_thread.start()

pipeIn = threading.Thread(target=simplePipeIn, args=(FIFO_IN,))
pipeIn.start()

window.wm_attributes('-fullscreen',"True")
window.mainloop()

pipeOut.join()
pipeIn.join()
