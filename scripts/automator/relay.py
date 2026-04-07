import curses
import hid
import time

# -------------------------
# Relay Control Function
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

def reset(device):
    relay_on(device, 1)
    time.sleep(0.5)
    relay_off(device, 1)

def download_mode(device):
    relay_on(device, 1)
    relay_on(device, 2)
    time.sleep(0.5)
    relay_off(device, 1)
    relay_off(device, 2)

# -------------------------
# Device Selection Screen
# -------------------------
def select_device_screen(stdscr):
    curses.curs_set(0)
    stdscr.clear()

    devices = hid.enumerate()
    if not devices:
        stdscr.addstr(0, 0, "No HID devices found.")
        stdscr.refresh()
        stdscr.getch()
        return None

    index = 0

    while True:
        stdscr.clear()
        stdscr.addstr(0, 0, "Select USB HID Device (Enter to select, q to quit)")

        for i, dev in enumerate(devices):
            name = f"{i}. {dev['path'].decode('utf-8')} Manufacturer={dev['manufacturer_string']} Product={dev['product_string']}"

            if i == index:
                stdscr.attron(curses.A_REVERSE)
                stdscr.addstr(i + 2, 2, name)
                stdscr.attroff(curses.A_REVERSE)
            else:
                stdscr.addstr(i + 2, 2, name)

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
    dev.open_path(device_info["path"])

    while True:
        stdscr.clear()
        stdscr.addstr(0, 0, "Relay Control")
        stdscr.addstr(2, 0, f"Device: {device_info['path'].decode('utf-8')} Manufacturer={device_info['manufacturer_string']} Product={device_info['product_string']}")
        stdscr.addstr(3, 0, "1. Reset")
        stdscr.addstr(4, 0, "2. Download Mode")
        stdscr.addstr(5, 0, "Press q to quit")

        stdscr.refresh()

        key = stdscr.getch()

        if key == ord("1"):
            reset(dev)

        elif key == ord("2"):
            download_mode(dev)

        elif key == ord("q"):
            break

    dev.close()


# -------------------------
# Main
# -------------------------
def main(stdscr):
    device_info = select_device_screen(stdscr)
    if device_info:
        relay_screen(stdscr, device_info)


if __name__ == "__main__":
    curses.wrapper(main)
