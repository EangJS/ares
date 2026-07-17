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

def tcp_client(host, port):
    sent_cnt = 0
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        try:
            # Connect to the server
            client_socket.connect((host, port))
            current_datetime = datetime.now()
            print(f'{current_datetime} TCP Client: Connected to TCP {host}:{port}')
            print('TCP Client: Start send')
            while True:
                message = generate_random_message()
                client_socket.sendall(message)
                # print(f'Sent (TCP): {message.decode("utf-8")}')
                current_datetime = datetime.now()
                sent_cnt += 1
                # print(f"{current_datetime} #{sent_cnt}: Sent TCP bytes: {len(message)}")
                # response = client_socket.recv(BUFFER_SIZE)
                # print(f'Received (TCP): {response.decode("utf-8")}')
                time.sleep(1/PACKET_INTERVAL) # Will affect the packet interval to send, in seconds
        except KeyboardInterrupt:
            print(f"Exit by user")
        except BrokenPipeError:
              print("TCP Client: Server closed the connection, test ended")
        except socket.error as e:
            print(f"Socket error: {e}")
        except Exception as e:
            print(f'TCP error occurred: {e}')
        except EOFError:
            print(f"Exit by EOF")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: tcp_client.py <IP_ADDRESS> [<PORT>]")
        sys.exit(1)

    CLIENT_HOST = sys.argv[1]    # The IP address provided by user
    TCP_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 5000  # Default TCP port 5000 if not provided
    tcp_client(CLIENT_HOST,TCP_PORT)
