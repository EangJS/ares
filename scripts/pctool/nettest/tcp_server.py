import socket
import threading
import time
import string
import random
import sys
import struct
from datetime import datetime

# Constants for message generation and buffer size
BUFFER_SIZE = 1024
HDR_SIZE = 4 # Header to show packet length

def recv_all(sock, n):
    #Read exactly n bytes from a TCP socket. Returns None on connection close.
    data = b""
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            return None  # connection closed
        data += chunk
    return data

def tcp_server(server_host, server_port):
    total_ok = 0
    total_fail = 0
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
                    # Step 1: Read the 4-byte length header
                    header = recv_all(conn, HDR_SIZE)
                    if header is None:
                        break  # connection closed
                    claimed_len = struct.unpack("<I", header)[0]  # little-endian uint32

                    # Sanity check: reject implausible lengths
                    if claimed_len == 0 or claimed_len > 65535:
                        print(f"{datetime.now()} ###FAIL: implausible length {claimed_len}###")
                        total_fail += 1
                        continue

                    # Step 2: Read exactly claimed_len bytes of payload
                    payload = recv_all(conn, claimed_len)
                    if payload is None:
                        print(f"{datetime.now()} ###FAIL: connection closed mid-packet (expected {claimed_len}B)###")
                        total_fail += 1
                        break

                    # Step 3: Verify length
                    if len(payload) == claimed_len:
                        total_ok += 1
                        # print(f"{datetime.now()} [TCP] ✅ #{total_ok} len={claimed_len} total={HDR_SIZE + claimed_len}B")
                    else:
                        total_fail += 1
                        print(f"{datetime.now()} [TCP] ❌ #{total_ok + total_fail} header={claimed_len}B actual={len(payload)}B")

                # Print summary when a client disconnects
                grand_total = total_ok + total_fail
                print(f"{datetime.now()} TCP Server: Client {addr} disconnected")
                print(f"{datetime.now()} ###VERDICT### OK={total_ok} FAIL={total_fail} TOTAL={grand_total}")
                # Reset stats for next connection
                total_ok = 0
                total_fail = 0

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: tcp_server.py <IP_ADDRESS> [<PORT>]")
        sys.exit(1)
    SERVER_HOST = sys.argv[1]    # The server's hostname or IP address
    TCP_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5000  # Default TCP port 5000 if not provided
    tcp_server(SERVER_HOST, TCP_PORT)
