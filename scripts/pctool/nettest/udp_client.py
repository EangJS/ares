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
PACKET_INTERVAL = 50

def generate_random_message(min_length=1, max_length=500):
    length = random.randint(min_length, max_length)
    payload = ''.join(random.choices(string.ascii_letters + string.digits, k=length)).encode('utf-8')
    header = struct.pack("<I", length)
    # print(f"  gen: payload={length}B total={HDR_SIZE + length}B")
    return header + payload

def udp_client(host, port):
    sent_cnt = 0
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client_socket:
        print("UDP Client Start!")
        # client_socket.settimeout(5) # May only be needed if we want to receive the server's response
        try:
            while True:
                # Send the message to the server
                current_datetime = datetime.now()
                message = generate_random_message()
                client_socket.sendto(message, (host, port))
                sent_cnt += 1
                # print(f"{current_datetime} #{sent_cnt}: Sent UDP bytes: {len(message)}")
                # Receive the server's response
                # data, addr = client_socket.recvfrom(BUFFER_SIZE)
                
                # print(f"Received response from {addr}: {data.decode('utf-8')}")
                time.sleep(1/PACKET_INTERVAL) # Will affect the packet interval to send, in seconds
        except KeyboardInterrupt:
            print(f"Exit by user")
        except socket.error as e:
            print(f"Socket error: {e}")
        except Exception as e:
            print(f'UDP error occurred: {e}')
        except EOFError:
            print(f"Exit by EOF")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: udp_client.py <IP_ADDRESS> [<PORT>]")
        sys.exit(1)
    CLIENT_HOST = sys.argv[1]    # The server's hostname or IP address
    UDP_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5001  # Default UDP port 5001 if not provided
    udp_client(CLIENT_HOST,UDP_PORT)
