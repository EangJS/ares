#!/usr/bin/python3

import time
import curses
import hid
import time
import glob
import os
import random

# -------------------------
# Relay Control Functions
# -------------------------
def _send(dev, cmd, relay):
    report = bytes([
        0x00,    # Report ID
        cmd,     # Command
        relay,   # Relay number
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    ])
    dev.send_feature_report(report)
    time.sleep(0.05)

def relay_on(dev, n):
    _send(dev, 0xFF, n)

def relay_off(dev, n):
    _send(dev, 0xFD, n)

def reset(dev):
    relay_on(dev, 1)
    time.sleep(0.5)
    relay_off(dev, 1)

def download_mode(dev):
    relay_on(dev, 1)
    relay_on(dev, 2)
    time.sleep(0.5)
    relay_off(dev, 1)
    relay_off(dev, 2)

# -------------------------
# Resolve udev name (/dev/USBRelay_*)
# -------------------------
def resolve_udev_name(hid_dev):
    """
    Map hidapi device -> /dev/hidrawX -> /dev/USBRelay_*
    """

    hid_path = hid_dev['path'].decode()
    usb_id = hid_path.split(':')[0]

    # find matching hidraw device
    for hidraw in os.listdir("/sys/class/hidraw"):
        sys_path = f"/sys/class/hidraw/{hidraw}/device"
        real = os.path.realpath(sys_path)

        if usb_id in real:
            hidraw_dev = f"/dev/{hidraw}"

            # find matching udev symlink
            for link in glob.glob("/dev/USBRelay_*"):
                if os.path.exists(link) and os.path.realpath(link) == os.path.realpath(hidraw_dev):
                    return os.path.basename(link)

    return hid_dev.get("product_string", "USBRelay")

# -------------------------
# Device List
# -------------------------
def list_relays():
    devices = []

    for d in hid.enumerate():
        if d['vendor_id'] == 0x16c0 and d['product_id'] == 0x05df:

            name = resolve_udev_name(d)

            devices.append({
                "name": name,
                "path": d['path'],
                "raw": d
            })

    return devices

# -------------------------
# Device Selection Screen
# -------------------------
def select_device_screen(stdscr):
    curses.curs_set(0)

    devices = list_relays()
    if not devices:
        stdscr.addstr(0, 0, "No USBRelay devices found.")
        stdscr.refresh()
        stdscr.getch()
        return None

    index = 0

    while True:
        stdscr.clear()
        stdscr.addstr(0, 0, "Select USB Relay (Enter to select, q to quit)")

        for i, dev in enumerate(devices):
            line = f"{i}. {dev['name']} @ {dev['path'].decode('utf-8')} Manufacturer={dev['raw']['manufacturer_string']} Product={dev['raw']['product_string']}"

            if i == index:
                stdscr.attron(curses.A_REVERSE)
                stdscr.addstr(i + 2, 2, line)
                stdscr.attroff(curses.A_REVERSE)
            else:
                stdscr.addstr(i + 2, 2, line)

        key = stdscr.getch()

        if key == curses.KEY_UP and index > 0:
            index -= 1
        elif key == curses.KEY_DOWN and index < len(devices) - 1:
            index += 1
        elif key == ord("\n"):
            return devices[index]
        elif key == ord("q"):
            return None

# -------------------------
# Relay Control Screen
# -------------------------
def relay_screen(stdscr, device_info):
    stdscr.clear()
    curses.curs_set(0)

    dev = hid.device()
    try:
        dev.open_path(device_info["path"])
    except Exception as e:
        print(f"Error: {e}\nPerhaps someone is using it?")
        time.sleep(1)
        return

    # Reset loop control
    reset_range = None
    next_reset_time = None
    current_delay = None

    def stop_reset_loop():
        nonlocal reset_range, next_reset_time, current_delay
        reset_range = None
        next_reset_time = None
        current_delay = None

    def schedule_next_reset():
        nonlocal next_reset_time, current_delay
        if reset_range is None:
            return
        current_delay = random.uniform(reset_range[0], reset_range[1])
        next_reset_time = time.monotonic() + current_delay

    def start_reset_loop(min_sec, max_sec):
        nonlocal reset_range
        stop_reset_loop()
        reset_range = (min_sec, max_sec)
        reset(dev)
        schedule_next_reset()

    # Do not block forever at getch(), so the reset loop can keep running.
    stdscr.timeout(200)

    while True:
        now = time.monotonic()

        if reset_range is not None and next_reset_time is not None and now >= next_reset_time:
            reset(dev)
            schedule_next_reset()

        stdscr.clear()
        stdscr.addstr(0, 0, "Relay Control")

        stdscr.addstr(
            2, 0,
            f"Device: {device_info['name']} ({device_info['path'].decode('utf-8')})"
        )

        if reset_range is None:
            status = "Reset loop: stopped"
        else:
            remain = max(0, next_reset_time - time.monotonic()) if next_reset_time else 0
            status = (
                f"Reset loop: running randomly every {reset_range[0]}~{reset_range[1]}s "
                f"(current delay: {current_delay:.2f}s, next in: {remain:.2f}s)"
            )

        stdscr.addstr(4, 0, status)
        stdscr.addstr(6, 0, "1. Reset once")
        stdscr.addstr(7, 0, "2. Download Mode")
        stdscr.addstr(8, 0, "3. Start reset loop randomly every 3~5s")
        stdscr.addstr(9, 0, "4. Start reset loop randomly every 7~9s")
        stdscr.addstr(10, 0, "s. Stop reset loop")
        stdscr.addstr(11, 0, "q. Quit")

        stdscr.refresh()

        key = stdscr.getch()

        if key == ord("1"):
            stop_reset_loop()
            reset(dev)

        elif key == ord("2"):
            stop_reset_loop()
            download_mode(dev)

        elif key == ord("3"):
            start_reset_loop(3, 5)

        elif key == ord("4"):
            start_reset_loop(7, 9)

        elif key == ord("s"):
            stop_reset_loop()

        elif key == ord("q"):
            stop_reset_loop()
            break

    dev.close()

# -------------------------
# Main
# -------------------------
def main(stdscr):
    while True:
        device_info = select_device_screen(stdscr)
        if device_info:
            relay_screen(stdscr, device_info)

if __name__ == "__main__":
    curses.wrapper(main)
