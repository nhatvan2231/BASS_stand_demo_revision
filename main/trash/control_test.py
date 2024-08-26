import os
import json
os.environ["MAVLINK20"] = "1"
from pymavlink.dialects.v20 import common as mavlink2
from dronekit import *

Q_motor_thread_start = False
Q_kill = False
DEFAULT_COM = "/dev/ttyACM0"
jab = ''


def set_msg_interval(vehicle, msg_id=30, interval=1e5):
    '''
    ARGS: vehicle - vehicle object; msg_id - ID of Mavlink message. See https://mavlink.io/en/messages/common.html;
    interval - interval in usec for message sending\n
    Requests the drone to send the mavlink message given by msg_id to at the specified interval.
    '''
    cmd = vehicle.message_factory.command_long_encode(0,0, mavutil.mavlink.MAV_CMD_SET_MESSAGE_INTERVAL, msg_id, interval, 0, 0, 0, 0, 0, 0)
    vehicle.send_mavlink(cmd)
def con(com="COM4", arm=False, baud=57600, mode="POSHOLD", replace_listeners=[]):
    '''
    ARGS: com - COM port which windows uses to recognize the serial radio; arm - arm the drone after connecting;
    baud - baud rate for the serial link; mode - desired mode to start flight in. "POSHOLD" ideal for indoor flight;
    replace-listeners - message listeners which should not be initiated by the con function. Use this in case the
    message listeners should have some special functionality, such as triggering a directory survey.\n
    RETURNS: vehicle object\n
    Initialized required message listeners... Attitude message updates the vehicles current yaw and yawspeed. Heartbeat is sent
    every time a heartbeat is received
    '''

    # Connect to the drone, create vehicle object
    vehicle = connect(com, baud=baud)
    # Send initalizing heartbeats to the drone (5 times for redundancy)
    heartbeat = vehicle.message_factory.heartbeat_encode(0, 0, 0, 0, 1)
    for i in range(0, 5):
        vehicle.send_mavlink(heartbeat)
        time.sleep(0.1)
    # Set the vehicle mode
    vehicle.mode = VehicleMode(mode)
    # Arm the vehicle
    if arm:
        vehicle.arm()
    # Do not constantly print the RC_CHANNELS messages
    vehicle.receive_rc = False
    vehicle.manual_override = 1094 # 1094=Dronekit control, 1888=RC control. This is an extra safety feature
    vehicle.receive_att = False

    vehicle.do_scan = True
    vehicle.new_target = False

    set_msg_interval(vehicle)

    if "ATTITUDE" not in replace_listeners:
        @vehicle.on_message('ATTITUDE')
        def listener(self, name, msg):
            # Every time an attitude message is received, update the yaw and yawspeed variables.
            heartbeat = vehicle.message_factory.heartbeat_encode(0, 0, 0, 0, 1)
            vehicle.send_mavlink(heartbeat)
            self.yawspeed = msg.yawspeed
            self.yaw = msg.yaw
            if vehicle.receive_att:
                print(msg)

    if "HEARTBEAT" not in replace_listeners:
        @vehicle.on_message('HEARTBEAT')
        def listener(self, name, msg):
            # Every time a heartbeat is received, send one back.
            heartbeat = vehicle.message_factory.heartbeat_encode(0, 0, 0, 0, 1)
            vehicle.send_mavlink(heartbeat)

    if "RC_CHANNELS" not in replace_listeners:
        @vehicle.on_message('RC_CHANNELS')
        def listener(self, name, msg):
            # Read the RC data from the controller
            self.manual_override = msg.chan6_raw
            if self.manual_override == 1888:
                raise ControlException

            if self.receive_rc:
                print(msg)

    return vehicle

#def motor_thread(com=DEFAULT_COM, MT_TIME=3600):
def motor_thread(vehicle, MT_TIME=3600):
    global Q_motor_thread_start
    global Q_kill
    
    if(Q_motor_thread_start == True):
        Q_kill = True
        Q_motor_thread_start = False
        print("Motor Test Off")
        exit
    else:
        Q_motor_thread_start = True
        Q_kill = False
       # vehicle = con(com=com)

        print("Connected")

#        vehicle.flush()

        motor_start(vehicle,MT_TIME)
        

        print("Motor Test On")
        while(True):
            if(Q_kill == True):
                motor_kill(vehicle)
                break
            else:
                time.sleep(0.01)
 #       vehicle.flush()
  #      vehicle.close()

def landing_gear(vehicle, up):
	print("LEGS ACTIVATED")
#	msg = vehicle.message_factory.command_long_encode(
#	   1,1,
#           mavutil.mavlink.MAV_CMD_AIRFRAME_CONFIGURATION,
#    	   0,
#   	   0,1,0,0,0,0,0)

	lg_pwm = 1900
	if up:
		lg_pwm = 1100

	print(lg_pwm)
	msg = vehicle.message_factory.rc_channels_override_encode(1,1,0,0,0,0,0,0,0,lg_pwm)
	vehicle.send_mavlink(msg)

def motor_start(vehicle,MT_TIME = 3600):
    landing_gear(vehicle, True)
    time.sleep(0.01)
    for i in range(5):
        msg = vehicle.message_factory.command_long_encode(
                    0,0,
                    mavutil.mavlink.MAV_CMD_DO_MOTOR_TEST,
                    0,
                    i,0,5,MT_TIME,0,0,0)
        vehicle.send_mavlink(msg)
        time.sleep(1)
 
def motor_kill(vehicle):
     landing_gear(vehicle, False)
     time.sleep(3)
     for i in range(5):
        msg = vehicle.message_factory.command_long_encode(
                    0,0,
                    mavutil.mavlink.MAV_CMD_DO_MOTOR_TEST,
                    0,
                    i,0,5,0,0,0,0)
        vehicle.send_mavlink(msg)
     time.sleep(1)


	
	

