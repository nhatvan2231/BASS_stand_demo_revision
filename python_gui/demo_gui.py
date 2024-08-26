#!/usr/bin/env python3
import tkinter as tk
import random
from tkinter import *
# import RPi.GPIO as GPIO
import time, sys
import threading
import os
import fcntl
import numpy as np
import struct
# from socket import *
# from control_test import *
import tkinter.messagebox
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, NavigationToolbar2Tk)
import asyncio
import struct
import queue

# NCPA_LOGO_FILE=os.environ["HOME"] + "/BASS_stand_demo_revision/python_gui/bass_stand_logo.png"
NCPA_LOGO_FILE = "./bass_stand_logo.png"

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

killWord = False
currentFrame = "one"

messageIn = np.zeros((MSG_IN_SIZE,), dtype=np.float64)
messageOut = np.zeros((MSG_OUT_SIZE,), dtype=np.uint16)
dataQueue = queue.Queue()


# # read FIFO messages
def simplePipeIn(FIFO, SIZE = 2):
    global messageIn, killWord
    with open(FIFO, 'rb', os.O_NONBLOCK) as pipe:
        while True:
            if killWord:
                return
            data = pipe.read(SIZE * np.dtype(np.float64).itemsize) # number of messages * datatype = number of bytes to read
            if len(data) == 0:
                time.sleep(0.001)
                continue
            messageIn = struct.unpack('{0}d'.format(SIZE),data)
            time.sleep(0.001)


# write FIFO messages
def simplePipeOut(FIFO, SIZE = 1):
    global killWord
    with open(FIFO, 'wb', os.O_NONBLOCK) as pipe:
        print(messageOut)
        data = struct.pack('{0}H'.format(SIZE), *messageOut)
        pipe.write(data)
    time.sleep(0.001)


# read fifo messages with data
def channelvisPipeIn():
    global killWord, dataQueue, label_battery_status_update
    pipe3 = open("/tmp/channelvis_FTP", "rb", os.O_NONBLOCK)
    while True:
        if killWord:
            return
        # label_battery_status_update.config(text = os.fstat(pipe3.fileno()).st_size)
        data3 = pipe3.read(4)
        if len(data3) == 0:
            time.sleep(0.01)
            pipe3 = open("/tmp/channelvis_FTP", "rb", os.O_NONBLOCK)
            continue
        else:
            # print("data3: ", data3)
            dataQueue.put(data3)


def purple_button():  # Initialize Button
    # TODO
    # play_tune(1)
    messageOut[0] = 1
    simplePipeOut(FIFO_OUT)


def green_button():  # Scan
    # TODO
    # play_tune(3)
    messageOut[0] = 2
    simplePipeOut(FIFO_OUT)


def yellow_button():  # Motor Test
    # TODO
    # play_tune(3)
    messageOut[0] = 3
    simplePipeOut(FIFO_OUT)


def play_tune(tune):
    pass
    # Tune:
    # 0: button press
    # 1: initialize
    # 2: low battery
    messageOut[2] = tune;
    simplePipeOut(FIFO_OUT)


def battery_callback():  # Battery status
    # TODO
    global killWord
    while (True):
        # print(messageIn[0])
        batt_voltage = messageIn[0]
        # label_battery_status_update.config(text="{0} V".format(batt_voltage), font=("Times", 16, 'bold'), relief=FLAT,
        #                                    fg='red', bg='#FFFFFF')
        # if batt_voltage <= 20.0:
        # messageOut[3] = 0
        # simplePipeOut(FIFO_OUT)
        # low_battery_voltage()
        if killWord:
            break
        time.sleep(1)


# pop up warning for low battery
def low_battery_voltage():
    # play_tune(2)
    warning = Toplevel(window, bg='red')
    warning.title('WARNING!')
    warning.wm_attributes('-fullscreen', "True")
    warning.geometry('800x480')
    canvas = tkinter.Canvas(warning, bg='black', width=800, height=480)
    canvas.create_polygon(-100, 0, 0, 0, -100, 480, -200, 480, fill='yellow', tags='polygon')
    canvas.create_polygon(100, 0, 200, 0, 100, 480, 0, 480, fill='yellow', tags='polygon')
    canvas.create_polygon(300, 0, 400, 0, 300, 480, 200, 480, fill='yellow', tags='polygon')
    canvas.create_polygon(500, 0, 600, 0, 500, 480, 400, 480, fill='yellow', tags='polygon')
    canvas.create_rectangle(0, 120, 750, 280, fill='yellow')
    canvas.create_text(300, 200, text='BATTERY LOW!\nUNPLUG BATTERY', justify=tk.CENTER, fill='red',
                       font=('times', 42, 'bold'))
    canvas.pack()
    # GPIO.cleanup()

    def first():
        canvas.configure(bg='yellow')
        canvas.itemconfig('polygon', fill='black')
        warning.after(300, second)

    def second():
        canvas.configure(bg='black')
        canvas.itemconfig('polygon', fill='yellow')
        warning.after(300, first)

    first()
    warning.mainloop()


# Setting up GUI Window
window = tk.Tk()
window.title("BOX-DRONE INTERFACE")
# window.state('zoomed')
window.geometry("640x480")
frame = Frame(window, bg='#FFFFFF', borderwidth=0)
# frame.pack(expand=1, fill = BOTH)
frame.grid(row=0, column=0, sticky='news')

frame2 = Frame(window, bg='#FFFFFF', borderwidth=0)
frame2.grid(row=0, column=0, sticky='news')

frame.tkraise()


def forceQuit():
    global window, killWord
    # window.destroy()
    killWord = True
    print("Force quitting!")
    sys.exit()
    raise SystemExit


# NCPA Image
# logo = PhotoImage(file = NCPA_LOGO_FILE)
# label_logo = Label(frame, image=logo, font = ("Times", 18), relief = 'flat',borderwidth = 0)
# label_logo.grid(row=0, column=0, columnspan = 3)
leftFrame = Frame(frame, background="#FFFFFF")
leftFrame.pack(side='left') #!!
topFrame = Frame(leftFrame, bg='#FFFFFF')
topFrame.pack(side="top", fill="y", expand=True)
#uncomment once I figure out why photos matter?
oleMissLogo = PhotoImage(file="/home/ncpa/BASS_stand_demo_harley/python_gui/OleMiss_Logo.png")
# oleMissLogo = oleMissLogo.zoom(3, 3)
# oleMissLogo = oleMissLogo.subsample(2, 2)
label_oleMissLogo = Label(topFrame, image=oleMissLogo, font=("Times", 18), relief='flat', borderwidth=0, bg='#FFFFFF')
# label_oleMissLogo.grid(row=0, column=6, columnspan=4)
label_oleMissLogo.pack(side="top", fill="both", expand=True)
ncpaLogo = PhotoImage(file="/home/ncpa/BASS_stand_demo_harley/python_gui/NCPA_logo.png")
# ncpaLogo = ncpaLogo.zoom(3, 3)
# ncpaLogo = ncpaLogo.subsample(2, 2)
label_ncpaLogo = Label(topFrame, image=ncpaLogo, font=("Times", 18), relief='flat', borderwidth=0, bg='#FFFFFF')
# label_ncpaLogo.grid(row=0, column=0, columnspan=6)
label_ncpaLogo.pack(side="top", fill="both", expand=True)


midFrame = Frame(leftFrame, bg='#FFFFFF')
midFrame.pack(side="top")
# Displaying Battery Status on GUI
label_battery = Label(midFrame, text='BATTERY STATUS: ', font=("Times", 16, 'bold'), relief=FLAT, fg='red', bg='#FFFFFF')
# label_battery.grid(row=6, column=0, columnspan=1, sticky="SW", pady=8)
# label_battery.pack(side="top", fill="x", expand=True)
label_battery_status_update = Label(midFrame, text=' ', font=("Times", 19, 'bold'), relief=FLAT, bg='#FFFFFF')
# label_battery_status_update.grid(row=6, column=1, columnspan=2, sticky="SEWE", pady=8)
# label_battery_status_update.pack(side="top", expand=True, fill="x")


# Displaying Current angle
current_angle = Label(midFrame, text="{0}{1}".format(messageIn[1], u"\u00b0"), font=("Times", 30), width=10, relief=RIDGE,
                      anchor=tk.CENTER, bg='#00FF00')
# current_angle.grid(row=1, column=1, pady=10)
# current_angle.pack(side="top")

# Constructing / Customizing Buttons
midFrameLeft = Frame(midFrame, bg='#FFFFFF')
midFrameRight = Frame(midFrame, bg='#FFFFFF')
midFrameLeft.pack(side="left")
midFrameRight.pack(side="right")

green = Button(midFrameLeft, text='ON/OFF', command=green_button, font=(16), height=3, relief=RAISED)
# green.grid(row=5, column=0, columnspan=2, pady=5, padx=0)
green.config(highlightbackground="green", highlightthickness=3)
green.pack(side="top", fill="x", padx=1, pady=1)

yellow = Button(midFrameLeft, text='HOVER ON/OFF', command=yellow_button, font=(16), height=3, relief=RAISED)
# yellow.grid(row=5, column=2, columnspan=2, pady=5, padx=0)
yellow.config(highlightbackground="yellow", highlightthickness=3)
yellow.pack(side="top", fill="x", padx=1, pady=1)

purple = Button(midFrameRight, text='INITIALIZE', command=purple_button, font=(16), height=3, relief=RAISED)
# purple.grid(row=5, column=4, columnspan=2, pady=5, padx=0)
purple.config(highlightbackground="magenta", highlightthickness=3)
purple.pack(side="top", fill="x", padx=1, pady=1)

# bottomFrame = Frame(leftFrame, bg='#FFFFFF')
# bottomFrame.pack(side="bottom")

# exitB = Button(bottomFrame, text="Quit", command=forceQuit, font=(16), activebackground='red', bg='red', height=3)
# exit.grid(row=5, column=6, columnspan=2, pady=5, padx=5)
# exitB.pack(side="top", expand=True, fill="x", padx=5)


def raiseFrame(num):
    global canvas, currentFrame, killWord2, readinBinaryThread, frame, frame2
    killWord2 = True
    # readinBinaryThread.join()
    print("Thread Quit")
    if(num == "one"):
        frame.tkraise()
        currentFrame="one"
    elif(num =="two"):
        frame2.tkraise()
        currentFrame="two"
    init_fig()
    # print("Current Frame: ",currentFrame)

def raiseFrameOne(event):
    raiseFrame("one")

raisef2 = Button(midFrameRight, text="FULLSCREEN", command=lambda:raiseFrame("two"), font=(16), height=3, relief=RAISED)
# raisef2.grid(row=5, column=8, columnspan=2, pady=5, padx=5)
raisef2.config(highlightbackground="red", highlightthickness=3)
raisef2.pack(side="top", fill="x", padx=1, pady=1)

# raisef1 = Button(frame2, text="Return", command=lambda:raiseFrame("one"), font=(16), activebackground="red",
#                  bg="red", height=3)
# # raisef1.grid(row=5, column=4)
# raisef1.pack(side="left")

f2rightFrame = Frame(frame2)
f2rightFrame.pack(side="right", expand=True, fill="x")

# battery_thread = threading.Thread(target=battery_callback)
# battery_thread.start()

rightFrame = Frame(frame)
rightFrame.pack(side="right")
fig, axes = plt.subplots(1, 1)
canvas = FigureCanvasTkAgg(fig, master=rightFrame)
# canvas.draw()
canvas.get_tk_widget().pack(fill='both', expand=True, side='top')
# bg = fig.canvas.copy_from_bbox(fig.bbox)
bg = 0
plots = []
totallength = int(48000 / (8))
appendlength = int(4800 / (8))
charts = np.zeros((1, totallength))
tempcharts = np.zeros((1, appendlength))
maxchannels = 1
numchannels = 1
framecount = 0
curselected=0

killWord2 = False
readinBinaryThread = -1

def readinbinary():
    global dataQueue, flushflag, faucet, decimationcount, decimationfactor, fig, axes, tempcharts, count, old_fig_size, bg, charts, plots, framecount, sleeptimer, latencyText, starttime, latencycount, maxchannels, curselected
    structstring = '@' + ('f' * maxchannels)
    # structsize = maxchannels * 8
    structsize = 1 * 4 #should be a single float coming in, i.e., maxchannels = 1, size of float is 4
    # print(structstring)
    totalcounter = 0
    count = 0
    # pipe = open("/tmp/channelvis_MTP", "rb", os.O_RDONLY | os.O_NONBLOCK) #!!
    while True:
        if killWord2:
            return
        nextpacket = None
        # nextpacket = read(structsize)
        # nextpacket = pipe.read(structsize) #!!
        if dataQueue.qsize() != 0:
            nextpacket = dataQueue.get()
            # print("Next packet: ", nextpacket)
            linesplit = struct.unpack(structstring, nextpacket) #!!
            # print(linesplit)
            # linesplit = [i/INT_MAX for i in linesplit]
            tempcharts[:, count] = np.asarray(linesplit, dtype=float) #!!
            # tempcharts[:, count] = np.asarray(noise.pnoise1(totalcounter), dtype=float)
            count += 1
            totalcounter += 0.00001
            if (count - appendlength == 0):
                fig.canvas.restore_region(bg)
                charts = np.concatenate((charts[:, appendlength:], tempcharts), axis=1)
                plots.set_data(xvals, charts[curselected])
                axes.draw_artist(plots)
                fig.canvas.blit(fig.bbox)
                fig.canvas.flush_events()
                count = 0


def init_fig():
    # print('init_fig')
    global readinBinaryThread, killWord2, bg, f2rightFrame, rightFrame, currentFrame, curselected, plots, totallength, fig, axes, xvals, ylim, numchannels, canvas, window, old_fig_size,rightFrame, totallength, appendlength, charts
    decimationfactor = 1
    plots = []
    xvals = range(totallength)
    width, height = (1, 1)
    plt.close()
    fig, axes = plt.subplots(nrows=height, ncols=width)
    ylim = .02
    # plt.connect('draw_event', on_draw)
    skipped = 0
    pltline, = axes.plot(charts[curselected], 'w', alpha=1, animated=True, zorder=10)
    # pltline, = axes.plot(charts[curselected], 'ok', ms=".25", ls='', alpha=1, animated=True, zorder=10)
    # pltline, = axes.plot(charts[0], animated=True, zorder=10)
    plots = pltline
    axes.set_xlim(0, totallength)
    axes.set_ylim(-1 * ylim, ylim)
    axes.draw_artist(pltline)
    axes.set_yticks([])
    axes.set_xticks([])
    axes.set_facecolor('xkcd:dark blue')
    # axes.set_title((0 + 1), fontsize='small', loc='left')

    plt.tight_layout()
    canvas.get_tk_widget().pack_forget()
    canvas.get_tk_widget().destroy()
    if (currentFrame == "one"):
        canvas = FigureCanvasTkAgg(fig, master=rightFrame)
        # fig.set_figheight(4.5)
        # fig.set_figwidth(4) #3.25

    elif (currentFrame == "two"):
        canvas = FigureCanvasTkAgg(fig, master=f2rightFrame)
        canvas.mpl_connect('button_press_event', raiseFrameOne)
        # fig.set_figheight(4.5)
        # fig.set_figwidth(5.925)
    canvas.draw()
    canvas.get_tk_widget().pack(fill='both', expand=True, side='right')
    bg = fig.canvas.copy_from_bbox(fig.bbox)
    killWord2 = False
    readinBinaryThread = threading.Thread(target=readinbinary)
    readinBinaryThread.start()


init_fig()







pipeIn = threading.Thread(target=simplePipeIn, args=(FIFO_IN,))
pipeIn.start()

pipeIn2 = threading.Thread(target=channelvisPipeIn)
pipeIn2.start()

# pipeOut = threading.Thread(target=simplePipeOut, args=(FIFO_OUT,))
# pipeOut.start()

window.wm_attributes('-fullscreen',"True")
window.mainloop()

# pipeOut.join()
pipeIn.join()
pipeIn2.join()
readinBinaryThread.join()
