# This script runs a Flask web server and a serial writer in parallel processes.
# It is meant to be run on a PC with UART and in the same network as the device running ares.

from multiprocessing import Process, Manager
from threading import Thread
import time
import random
from flask import Flask, jsonify, request
from serial import Serial
from time import sleep
import argparse
import sys
from nettest.tcp_client import tcp_client
from nettest.tcp_server import tcp_server
from nettest.udp_client import udp_client
from nettest.udp_server import udp_server

manager = Manager()
client_ips = manager.dict()

# -------- Flask App --------
app = Flask(__name__)

@app.route("/now/", methods=["GET"])
def get_time():
    current_time = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
    print(f"Requested from client: {request.remote_addr}")
    client_ips[request.remote_addr] = time.time()
    return jsonify({"time": current_time})

def run_flask():
    # Disable reloader for multiprocessing compatibility
    app.run(host="0.0.0.0", port=5000, debug=True, use_reloader=False)

# -------- Serial Writer --------
def run_serial(port, baudrate):
    try:
        ser = Serial(port, baudrate)
        print(f"Opened serial port {port} with baudrate {baudrate}")
        while True:
            sleep(random.uniform(0.5, 2.0))
            ser.write(b'1234567890abcdefghijklmnopqrstuvwxyz\0')
    except Exception as e:
        print(f"Serial error: {e}", file=sys.stderr)
        
def run_nettest_server(tcp_port, udp_port):
    tcp_server_thread = Thread(target= tcp_server, args=("0.0.0.0", tcp_port))
    udp_server_thread = Thread(target= udp_server, args=("0.0.0.0", udp_port))
    
    tcp_server_thread.start()
    udp_server_thread.start()
    
    tcp_server_thread.join()
    udp_server_thread.join()

def run_nettest_client(tcp_port, udp_port):
    while True:
        for ip, timestamp in client_ips.items():
            if time.time() - timestamp > 60:
                print(f"Client {ip} timed out.")
                del client_ips[ip]
                continue
            try:
                print(f"Running nettest client to {ip}")
                tcp_client(ip, tcp_port)
                udp_client(ip, udp_port)
            except Exception as e:
                print(f"Nettest client error for {ip}: {e}", file=sys.stderr)
        sleep(0.1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run serial ports and Flask server")

    ports = []
    for i in range(1, 4):
        parser.add_argument(
            f'--port{i}',
            type=str,
            required=False,
            help=f'Serial port {i}'
        )
        ports.append(f'port{i}')

    parser.add_argument('--baudrate', type=int, default=115200, help='Baud rate')
    parser.add_argument('--tcp-port', type=int, required=False, help='TCP Port for nettest')
    parser.add_argument('--udp-port', type=int, required=False, help='UDP Port for nettest')
    args = parser.parse_args()

    serial_processes = []
    for port_name in ports:
        port_value = getattr(args, port_name)
        if port_value:
            p = Process(target=run_serial, args=(port_value, args.baudrate))
            serial_processes.append(p)
            p.start()

    flask_process = Process(target=run_flask)
    flask_process.start()

    nettest_server_process = Process(target=run_nettest_server, args=(args.tcp_port, args.udp_port))
    nettest_server_process.start()
    
    nettest_client_process = Process(target=run_nettest_client, args=(args.tcp_port, args.udp_port))
    nettest_client_process.start()

    nettest_server_process.join()
    nettest_client_process.join()

    # Wait for all processes to finish
    for p in serial_processes:
        p.join()
    flask_process.join()
