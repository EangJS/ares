#!/usr/bin/env python
# -*- coding: utf-8 -*-
import hid
import time

VID = 0x16C0
PID = 0x05DF

class USBRelay:
    def __init__(self):
        self.dev = hid.device()
        self.dev.open(VID, PID)
        self.dev.set_nonblocking(False)

        print("Opened:",
              self.dev.get_manufacturer_string(),
              self.dev.get_product_string())

    def _send(self, cmd, relay):
        report = bytes([
            0x00,    # Report ID
            cmd,     # Command
            relay,   # Relay number
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            ])
        self.dev.send_feature_report(report)
        time.sleep(0.05)

    def relay_on(self, n):
        self._send(0xFF, n)

    def relay_off(self, n):
        self._send(0xFD, n)

    def all_on(self):
        self._send(0xFE, 0x00)

    def all_off(self):
        self._send(0xFC, 0x00)

    def close(self):
        self.dev.close()
        
def download_mode(r: USBRelay):
    r.relay_on(1)
    r.relay_on(2)
    time.sleep(0.5)
    r.relay_off(1)
    r.relay_off(2)

def reset(r: USBRelay):
    r.relay_on(1)
    time.sleep(0.5)
    r.relay_off(1)

def main():
    while True:
        r = USBRelay()
        cmd = input("Enter command (on/off <n>) (reset) (download) (exit): ").strip().lower()
        if cmd == "exit":
            break
        elif cmd == "download":
            download_mode(r)
        elif cmd == "reset":
            reset(r)
        elif cmd.startswith("on "):
            n = int(cmd.split()[1])
            r.relay_on(n)
        elif cmd.startswith("off "):
            n = int(cmd.split()[1])
            r.relay_off(n)
        else:
            print("Unknown command.")
        r.close()

if __name__ == "__main__":
    main()
