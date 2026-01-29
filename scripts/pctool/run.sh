#!/bin/bash

##### Add your configuration here #####
# SERIAL_PORT_X is optional, you can define up to 3 serial ports
# BAUDRATE is the baud rate for serial communication
# DUT_IP is the IP address of the Device Under Test (DUT) for nettest
# TCP_PORT and UDP_PORT are the ports used for nettest communication
# If TCP_PORT == UDP_PORT, both protocols will use the same port and
# -p is only specified once in docker run

FLASK_PORT=5000
TCP_PORT=5555
UDP_PORT=5555
BAUDRATE=115200
SERIAL_PORT_1=/dev/ttyACM0
SERIAL_PORT_2=/dev/ttyACM1
SERIAL_PORT_3=/dev/ttyACM2

##### End of configuration #####

docker run --rm -d \
  -p $TCP_PORT:$TCP_PORT \
  -p $FLASK_PORT:$FLASK_PORT \
  --name timeserver-app \
  --device=$SERIAL_PORT_1 \
  --device=$SERIAL_PORT_2 \
  --device=$SERIAL_PORT_3 \
  timeserver-app:latest \
  --port1=$SERIAL_PORT_1 \
  --port2=$SERIAL_PORT_2 \
  --port3=$SERIAL_PORT_3 \
  --baudrate=$BAUDRATE \
  --tcp-port=$TCP_PORT \
  --udp-port=$UDP_PORT
