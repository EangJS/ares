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

def tcp_server(server_host, server_port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((server_host, server_port))
        server_socket.listen()
        current_datetime = datetime.now()
        print(f"{current_datetime} TCP server listening on {server_host}:{server_port}")
        while True:
            conn, addr = server_socket.accept()
            with conn:
                current_datetime = datetime.now()
                print(f"{current_datetime} TCP connection established with {addr}")
                while True:
                    data = conn.recv(BUFFER_SIZE)
                    current_datetime = datetime.now()
                    if not data:
                        break
                    # print(f"Received from {addr}: {data.decode('utf-8')}")
                    print(f"{current_datetime} [TCP]Received from {addr}: {len(data)} bytes")
                    # conn.sendall(data) # uncomment if need to echo back

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: tcp_server.py <IP_ADDRESS> [<PORT>]")
        sys.exit(1)
    SERVER_HOST = sys.argv[1]    # The server's hostname or IP address
    TCP_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5000  # Default TCP port 5000 if not provided
    tcp_server(SERVER_HOST, TCP_PORT)
