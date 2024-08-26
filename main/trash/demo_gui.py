#!/bin/df/env python                         
import tkinter as tk
from tkinter import * 
import RPi.GPIO as GPIO 
import time
import threading
import os
import fcntl
from socket import *
from control_test import *
import tkinter.messagebox

vehicle = con(com = '/dev/ttyUSB0')
vehicle_parameters = Parameters(vehicle)
time.sleep(0.1)

motor_kill(vehicle)
time.sleep(1)

# 1200 steps per rotation
speed = 1  # deg/sec
scan_sleep = 0.3/speed

Q_pause_scan = False

#################################
########### Charlie #############
#################################

SERVER_angle = 0
SERVER_detected = False
SERVER_latency = 0          # measured in milliseconds
SERVER_triggered = False
trig_position = 0
angle_position = 0

master_step_counter = 0
Q_ms_counter = False

# zylia led stuff
cmd_setmode = 0x00000203
cmd_setcolor = 0x00000205

led_auto = [0]
led_manual = [1]

led_red = [255, 0, 0]
led_green = [0, 255, 0]
led_blue = [0, 0, 255]
led_yellow = [255, 255, 0]

while(not os.path.exists('/dev/zylia-zm-1_0')):
    print('waiting for zylia')
fd = os.open('/dev/zylia-zm-1_0', os.O_WRONLY)
# send the command for manual control
fcntl.ioctl(fd, cmd_setmode, bytes(led_manual))

# send a color
fcntl.ioctl(fd, cmd_setcolor, bytes(led_green))

#################################
########### Charlie #############
#################################

FILE_angle = 0
POSITION = 0
step_min = -550
step_max = 550
Q_scan = True
Q_kill
Q_motor_thread_start
Q_green = False
File_detected = False

#battery
batt_stat = ''
batt_low_volt = 0
#batt_low_volt = vehicle_parameters.__getitem__('BATT_CRT_VOLT')
if batt_low_volt < 20:
    batt_low_volt = 20

batt_cvolt = 0


#Setting up GUI Window
window = tk.Tk()
window.title("BOX-DRONE INTERFACE")
window.geometry("800x480")

frame = Frame(window, bg = '#FFFFFF', borderwidth = 0)
frame.pack(expand= 1, fill = BOTH)

#NCPA Image
logo = PhotoImage(file = "/home/ncpa/programming/bass_stand_demo/bass_stand_logo.png")
label_logo = Label(frame, image=logo, font = ("Times", 18), relief = 'flat',borderwidth = 0)
label_logo.grid(row=0, column=0, columnspan = 3)



#################################
########### Charlie #############
#################################

server_is_init = False
# server will listen for detects and angles
def server_thread(address, port):
    global fd, SERVER_angle, SERVER_detected, SERVER_latency, POSITION, Q_ms_counter, master_step_counter
    global Q_pause_scan

    # server settings & setup
    serverSocket = socket(AF_INET, SOCK_STREAM)
    serverSocket.bind((address, port))
    serverSocket.listen(1)

    server_is_init = True
    print("Server ready")

    # loop for receiving request
    while True:
        # wait for a client to connect
        connectionSocket, addr = serverSocket.accept()
        message = connectionSocket.recv(1024).decode()
        connectionSocket.close()
        print("Message received: " + message)

        # parse the message
        spl_msg = message.split(',')
            
        if spl_msg[0] == 'D':
            Q_pause_scan = True
            print("Detection!")
            trig_position = POSITION
            Q_ms_counter = True
            master_step_counter = False
            # send a color
            fcntl.ioctl(fd, cmd_setcolor, bytes(led_yellow))
            
        elif spl_msg[0] == 'A' and not SERVER_detected:
            print("Angle: " + spl_msg[3] + " " + spl_msg[4])
            SERVER_angle = float(spl_msg[3])
            Q_pause_scan = False
            if(SERVER_angle > 180 or SERVER_angle < -180):
                # send a color
                fcntl.ioctl(fd, cmd_setcolor, bytes(led_green))
                spl_msg[0] = 'X'
            else:
                # calculate and set SERVER_latency
                SERVER_latency = int(spl_msg[1]) - int(spl_msg[2])
                print("Latency = " + str(SERVER_latency))
                
                SERVER_detected = True
                angle_position = POSITION
            
        else:
            print("Invalid message!");

        print()

#################################
########### Charlie #############
#################################


GPIO.setwarnings(False)


def setup_board():
    GPIO.setmode(GPIO.BOARD)
    GPIO.setwarnings(False)
    GPIO.setup(18, GPIO.OUT,initial = GPIO.HIGH) #PUL
    GPIO.setup(12, GPIO.OUT,initial = GPIO.HIGH) #DIR
    GPIO.setup(15, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
    
def update_text():
    global step_min, step_max
    display_theta = round(abs(step_max-step_min)*0.9/6)
    output_text = "+/- " + str(display_theta) + " deg"
    label.config(text=output_text,font=("Times", 20, 'bold'))
  

def update_speed():
    global scan_sleep
    speed_text = round(0.3/scan_sleep)
    output_text = str(speed_text) + " deg/sec"
    label_speed.config(text=output_text,font=("Times", 20, 'bold'))
def speed_faster():
    global scan_sleep, speed
    if speed < 25:
        speed += 1
        scan_sleep = 0.3/speed
    update_speed()
def speed_slower():
    global scan_sleep, speed
    if speed > 1:
        speed -= 1
        scan_sleep = 0.3/speed
    update_speed()

#Inputing an Angle on GUI
def check_num():
    global Q_scan
    try:
        Q_scan = False
        theta = int(entry.get()) 
        angle_rotate(theta)
        time.sleep(1)
        Q_scan = True
    except ValueError:
        print("NOT a Number")


def pulse(direction, sleep_time = 0.01):
    global POSITION, step_min, step_max, Q_ms_counter, master_step_counter
    global Q_pause_scan

    if not Q_pause_scan:
        GPIO.output(12, direction)
        GPIO.output(18,1)
        time.sleep(sleep_time)
        GPIO.output(18,0)
        if direction == 0:
            POSITION -= 1
            if Q_ms_counter:
                master_step_counter -= 1
        if direction == 1:
            POSITION += 1
            if Q_ms_counter:
                master_step_counter += 1
        # +,- steps coordinate fix
        while POSITION > 600:
            POSITION -= 1200
        if POSITION < -600:
            POSITION += 1200
    else:
        time.sleep(sleep_time)



#Scanning
def scan():
    global step_min, step_max, POSITION, SERVER_angle, SERVER_detected, scan_sleep
    step_min = - 550 #LOWER_BOUND
    step_max =  550 #UPPER_BOUND

    slow_min = step_min + 6000*scan_sleep
    slow_max = step_max - 6000*scan_sleep
    
    print("Scanning...")

    update_text()
    while Q_green and Q_scan:
        while POSITION < step_max and Q_scan:
            if slow_min < POSITION or POSITION < slow_max:
                pulse(0, scan_sleep)
            else:
                pulse(0, (1 + abs(POSITION)/step_max)*scan_sleep)
        
            if SERVER_detected:
                SERVER_detected = False
                # send a color
                fcntl.ioctl(fd, cmd_setcolor, bytes(led_red))
                angle_rotate(SERVER_angle)
                time.sleep(5)
                # send a color
                fcntl.ioctl(fd, cmd_setcolor, bytes(led_green))
        
        time.sleep(50*scan_sleep)
        while POSITION > step_min and Q_scan:
            if slow_min < POSITION or POSITION < slow_max:
                pulse(1, scan_sleep)
            else:
                pulse(1, (1 + abs(POSITION)/step_max)*scan_sleep)

            if SERVER_detected:
                SERVER_detected = False
                # send a color
                fcntl.ioctl(fd, cmd_setcolor, bytes(led_red))
                angle_rotate(SERVER_angle)
                time.sleep(5)
                # send a color
                fcntl.ioctl(fd, cmd_setcolor, bytes(led_green))
        
        time.sleep(50*scan_sleep)
        print(abs(step_max-step_min)*0.9/3)





def deg_grow(): #+5 degrees
    global step_max, step_min
    print("RED")
    
    if step_max >= 550 or step_min <= -550: #sets a limit of 180 degrees as scan
        print("Limit Reached")
    else:
        step_max += 50/3
        step_min -= 50/3
        time.sleep(0.1)
    update_text()
        
def deg_shrink(): #-5 degrees
    global step_max, step_min, scan_sleep
    print("BLUE")
    if step_max <= 6000*scan_sleep or step_min >= 6000*scan_sleep: #sets limit to the smallest scan
        print("Limit Reached")
    else:
        step_max -= 50/3
        step_min += 50/3
        time.sleep(0.1)
    update_text()
        
def purple_button():   #Initialize Button
    print("PURPLE")
    setup_board()
    time.sleep(1)
    initialize()

        
def green_button(): 
    global Q_green, POSITION
    print("Green")
    thread1 = threading.Thread(target=scan)
    if Q_green:
        print("OFF")
        Q_green = False
        GPIO.cleanup()
        os.system("pkill 'run' &")
    else:                                   #ON
        print("ON")
        os.system("/home/ncpa/programming/zdaq_doa/run &")
        setup_board()
        time.sleep(0.5)
        angle_rotate(0)
        time.sleep(1)
        print("scan")
        Q_green = True
        thread1.start()
        
def yellow_button(): #Motor Test
    print("Yellow")
    thread2 = threading.Thread(target = motor_thread, args = (vehicle,))
    thread2.start()


def angle_rotate(noah_angle):
    global fd, POSITION, SERVER_latency, speed, SERVER_triggered, trig_position, angle_position, master_step_counter, Q_ms_counter

    angle_sleep = 0.01 # seconds
    
    # account for latency between detection and angle
    Q_ms_counter = False
    adj_angle = noah_angle - master_step_counter*0.3
    print("old angle = " + str(noah_angle) + "\tadj angle = " + str(adj_angle))

    noah_step = 3*round(adj_angle / 0.9)
    noah_step += POSITION
    
    print("noah_step = " + str(noah_step))

    while noah_step > 600:
        print(True)
        noah_step = noah_step - 1200
    while noah_step < -600:
        noah_step = 1200 + noah_step

    while POSITION < noah_step:
        pulse(1, angle_sleep)
    while POSITION > noah_step:
        pulse(0, angle_sleep)


def initialize():          #ROTATING_CLOCKWISE_UNTIL_LIMIT_SWITCH_IS_HIT
    global POSITION, fd
    setup_board()
    init_sleep = 0.01
    tune = b'MFT200L16<cdefgab>cdefgab.c'
    msg2 = mavutil.mavlink.MAVLink_play_tune_message(0,0, tune, tune2 = b'')
    vehicle.send_mavlink(msg2)
    print("***Initializing Motor***")
    while True:
        pulse(0, init_sleep)
        if GPIO.input(15) == GPIO.HIGH:
            print("Limit Switch Found!")
            POSITION = 0
            time.sleep(0.5)
            angle_rotate(177)
            time.sleep(0.5)
            POSITION = 0
            center = 0
            break
        update_speed()
    print("Awaiting Scan...")
    
    # begin server thread
    if not server_is_init:
        serv_thread = threading.Thread(target=server_thread, args=('127.0.0.1', 12000))
        serv_thread.start()

#Displaying Angle on GUI
label = Label(frame, text = 0, font = ("Times", 18), relief= FLAT, bg= '#FFFFFF')
label.grid(row=1, column=1, pady=11, padx = 40)

#Displaying Battery Status on GUI
label_battery = Label(frame, text = 'BATTERY STATUS:', font = ("Times", 16,'bold'), relief= FLAT, fg = 'red', bg = '#FFFFFF')
label_battery.grid(row=6, column=0, columnspan = 1,sticky ="SW", pady = 8)        
label_battery_status_update = Label(frame, text = ' ', font = ("Times", 19,'bold'), relief= FLAT, bg = '#FFFFFF')
label_battery_status_update.grid(row=6, column=1, columnspan =2, sticky = "SEWE", pady = 8)

#Displaying Scan Speed on GUI
label_speed = Label(frame, text = 0, font = (12), relief= FLAT, bg = '#FFFFFF')
label_speed.grid(row=4,column=1, pady=5, padx = 10)
butt_faster = Button(frame, text = '+DEG/SECOND', command = speed_faster, font = (16), activebackground = "orange", bg = "orange", height = 3)
butt_faster.grid(row=4, column = 2, pady=5, padx = 10)
butt_slower = Button(frame, text = '-DEG/SECOND', command = speed_slower, font = (16), activebackground = "orange", bg = "orange", height = 3)
butt_slower.grid(row=4, column = 0, pady=5, padx = 10)

#Constructing / Customizing Buttons
butt_grow = Button(frame, text = '   + DEGREES   ',  font = (16), command = deg_grow, activebackground = "red", bg = "red", height = 3)
butt_grow.grid(row=1, column = 2, pady=5, padx = 10)
butt_shrink = Button(frame, text = '   - DEGREES   ', command = deg_shrink, font = (16), activebackground = "red", bg = "red", height = 3)
butt_shrink.grid(row=1, column = 0, pady=5, padx = 10)
green = Button(frame, text = '     ON/OFF     ', command =  green_button, font = (16), activebackground = "green", bg = "green", height = 3)
green.grid(row=5, column = 0, pady=5, padx = 10)
yellow = Button(frame, text = 'HOVER ON/OFF', command = yellow_button, font = (16), activebackground = "yellow",  bg = "yellow", height = 3) 
yellow.grid(row=5, column = 1, pady=5, padx = 10)
purple = Button(frame, text = '    INITIALIZE    ', command = purple_button, font = (16), activebackground = 'magenta', bg = 'magenta', height = 3)
purple.grid(row=5, column = 2, pady=5, padx = 10)

#@vehicle.on_attribute('battery')
def battery_callback(self,attr_name,value):
    global batt_stat, batt_warning, batt_cvolt
    batt_stat = str(value)
    batt = batt_stat.split(":")
    batt = batt[1]
    batt = batt.split(",")
    batt_voltage = batt[0]
    batt_current = batt[1] 

    volt_dot_index = batt_voltage.find('.')
    if volt_dot_index != -1:
    	batt_voltage = batt_voltage[:volt_dot_index+2]
    
    current_dot_index = batt_current.find('.')
    if current_dot_index != -1:
        batt_current = batt_current[:current_dot_index+2]
        
    label_battery_status_update.config(text = batt_voltage+ "V" +"       " + batt_current+"A")
    time.sleep(0.1)

    batt_cvolt = batt_voltage.split('=')
    batt_cvolt = float(batt_cvolt[1])
    if batt_cvolt <= batt_low_volt:
        motor_kill(vehicle)
        vehicle.remove_attribute_listener('battery', battery_callback)
        time.sleep(0.1)
        low_battery_voltage()
 
vehicle.add_attribute_listener('battery', battery_callback)

#pop up warning for low battery
def low_battery_voltage():
    LOW = b'MBT60L4<<<<B'
    msg_low = mavutil.mavlink.MAVLink_play_tune_message(0,0, LOW, tune2 = b'')
    vehicle.send_mavlink(msg_low)
    
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


update_text()
update_speed()

window.wm_attributes('-fullscreen',"True")
window.mainloop()
