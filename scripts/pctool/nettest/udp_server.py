import socket
import threading
import time
import string
import random
import sys
from datetime import datetime

# Constants for message generation and buffer size
BUFFER_SIZE = 1024

def generate_random_message(min_length=1, max_length=500):
    length = random.randint(min_length, max_length)
    return ''.join(random.choices(string.ascii_letters + string.digits, k=length))

def udp_server(server_host, server_port):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server_socket:
        server_socket.bind((server_host, server_port))
        current_datetime = datetime.now()
        print(f"{current_datetime} UDP server listening on {server_host}:{server_port}")
        while True:
            data, addr = server_socket.recvfrom(BUFFER_SIZE)
            current_datetime = datetime.now()
            # print(f"Received from {addr}: {data.decode('utf-8')}")
            print(f"{current_datetime} [UDP]Received from {addr}: {len(data)} bytes")
            # server_socket.sendto(data, addr) # uncomment if need to echo back

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: udp_server.py <IP_ADDRESS> [<PORT>]")
        sys.exit(1)
    SERVER_HOST = sys.argv[1]    # The server's hostname or IP address
    UDP_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5001  # Default UDP port 5001 if not provided
    udp_server(SERVER_HOST,UDP_PORT)