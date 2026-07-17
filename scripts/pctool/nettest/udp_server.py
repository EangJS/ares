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
HDR_SIZE = 4
# TEST_PACKET_NO = 1200

def udp_server(server_host, server_port):
    total_ok = 0
    total_fail = 0
    recv_count = 0
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server_socket:
        server_socket.bind((server_host, server_port))
        # server_socket.settimeout(10)  # No timeout, let server keep running
        current_datetime = datetime.now()
        print(f"{current_datetime} UDP server listening on {server_host}:{server_port}")
        while True:
            # try:
            #     data, addr = server_socket.recvfrom(BUFFER_SIZE)
            # except socket.timeout:
            #     print(f"{datetime.now()} Timeout — no data for 10s, ending test")
            #     break
            data, addr = server_socket.recvfrom(BUFFER_SIZE)
            recv_count += 1
            # Parse the 4-byte length header
            claimed_len = struct.unpack("<I", data[:HDR_SIZE])[0]
            actual_payload = len(data) - HDR_SIZE
            if claimed_len == actual_payload:
                total_ok += 1
                # print(f"{datetime.now()} [UDP Server] ✅ #{total_ok} from {addr} "
                #     f"header={claimed_len} actual={actual_payload}")
            else:
                total_fail += 1
                print(f"{datetime.now()} [UDP Server] ❌ #{total_ok + total_fail} from {addr} "
                    f"###FAIL header={claimed_len} actual={actual_payload} ")
            # server_socket.sendto(data, addr) # uncomment if need to echo back

        # Final verdict
        if total_fail:
            print(f"{datetime.now()} ###UDP Server FAIL### OK={total_ok} FAIL={total_fail}")
        else:
            print(f"{datetime.now()} ###UDP Server PASS### OK={total_ok}")
        # Uncomment if want to calculate udp loss
        # udp_loss = ((TEST_PACKET_NO - recv_count)/(TEST_PACKET_NO) * 100)
        # print(f"UDP Server: Packet loss:{udp_loss:.2f}%")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: udp_server.py <IP_ADDRESS> [<PORT>]")
        sys.exit(1)
    SERVER_HOST = sys.argv[1]    # The server's hostname or IP address
    UDP_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5001  # Default UDP port 5001 if not provided
    udp_server(SERVER_HOST,UDP_PORT)