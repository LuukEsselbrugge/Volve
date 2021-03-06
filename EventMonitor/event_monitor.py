import serial
import os
import pyudev
import subprocess
import _thread
import platform
import subprocess
import time

def main():
	_thread.start_new_thread(usb_monitor, ())
	ser = serial.Serial(port='/dev/serial0',baudrate=115200,timeout=1)
	print("connected to: " + ser.portstr)
	count=1

	while True:
		#ser.write(b'getCommands\n');
		line = ser.readline()
		if line==b'EVENT_SHUTDOWN\n':
			os.system('shutdown -h now')
		if line==b'EVENT_UP\n':
			os.system('export DISPLAY=:0.0 && xdotool search --class autoapp key --window %@ Up')
		if line==b'EVENT_DOWN\n':
			os.system('export DISPLAY=:0.0 && xdotool search --class autoapp key --window %@ Down')
		if line==b'EVENT_LEFT\n':
			os.system('export DISPLAY=:0.0 && xdotool search --class autoapp key --window %@ 1')
		if line==b'EVENT_RIGHT\n':
			os.system('export DISPLAY=:0.0 && xdotool search --class autoapp key --window %@ 2')
		if line==b'EVENT_ENTER\n':
			os.system('export DISPLAY=:0.0 && xdotool search --class autoapp key --window %@ Return')
		if line==b'EVENT_BACK\n':
			os.system('export DISPLAY=:0.0 && xdotool search --class autoapp key --window %@ h')
	ser.close()
	
def usb_monitor():
	context = pyudev.Context()
	monitor = pyudev.Monitor.from_netlink(context)
	monitor.filter_by(subsystem='usb')
	monitor.start()
	add = 0
	remove = 0
	deviceID = ""
	for device in iter(monitor.poll, None):
		vendor = device.get('ID_VENDOR_FROM_DATABASE')
		if(deviceID == "" and vendor == "Google Inc."):
			deviceID = device.get('DEVNAME')	
		if device.get('DEVNAME') == deviceID:
			print("detected Android Phone")
			if device.action == "add" and add == 0:
				os.system('bash display_up.sh')
				print("up")
				add=1
				remove=0
			if device.action == "remove" and remove == 0:
				os.system('bash display_down.sh')
				print("down")
				remove=1
				add=0
				deviceID = ""

if __name__ == '__main__':
    main()
