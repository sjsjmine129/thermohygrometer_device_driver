cmd_/home/pi/Desktop/DeviceDriver/cDev.mod := printf '%s\n'   cDev.o | awk '!x[$$0]++ { print("/home/pi/Desktop/DeviceDriver/"$$0) }' > /home/pi/Desktop/DeviceDriver/cDev.mod
