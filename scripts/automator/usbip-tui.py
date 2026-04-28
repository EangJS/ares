#!/usr/bin/env python

import curses
import subprocess
import re
import os
import sys
import time
import json
import glob

RECENT_IPS_FILE = os.path.expanduser("~/.usbip_recent_ips.json")
MAX_RECENT = 10


# ============================================================
# SERVER SIDE
# ============================================================

def get_udev_name_from_sysfs(usb_busid):
    """
    Map busid like 1-1.4.3 → /dev/USBRelay_Left or /dev/FTDI_A
    """

    sysfs_path = f"/sys/bus/usb/devices/{usb_busid}"

    if not os.path.exists(sysfs_path):
        return None

    # find hidraw or tty device under this USB path
    for root, dirs, files in os.walk(sysfs_path):
        for d in dirs:
            if d.startswith("hidraw") or d.startswith("ttyUSB"):
                dev_path = f"/dev/{d}"

                # resolve udev symlink
                for link in glob.glob("/dev/*"):
                    if os.path.islink(link) and os.path.realpath(link) == os.path.realpath(dev_path):
                        return os.path.basename(link)

    return None

def parse_local_list(output):
    devices = []
    lines = output.splitlines()

    busid_pattern = re.compile(r"- busid (\S+) \((.*?)\)")
    name_pattern = re.compile(r"\s*(.+?)\s*:\s*(.+?)\s*\(")

    current = None

    for line in lines:
        busid_match = busid_pattern.search(line)
        if busid_match:
            if current:
                devices.append(current)

            current = {
                "busid": busid_match.group(1),
                "vidpid": busid_match.group(2),
                "name": "",
                "bound": False,
            }
            continue

        if current:
            name_match = name_pattern.search(line)
            if name_match:
                udev_name = get_udev_name_from_sysfs(current["busid"])
                manufacturer = name_match.group(1).strip()
                product = name_match.group(2).strip()
                current["name"] = f"{manufacturer} - {product} - {udev_name}"

    if current:
        devices.append(current)

    return devices


def get_bound_devices():
    driver_path = "/sys/bus/usb/drivers/usbip-host"
    bound = set()

    try:
        for entry in os.listdir(driver_path):
            if "-" in entry and entry[0].isdigit():
                bound.add(entry)
    except (FileNotFoundError, PermissionError):
        pass

    return bound


def get_local_devices():
    try:
        result = subprocess.run(
            ["usbip", "list", "-l"],
            capture_output=True,
            text=True,
            check=True
        )

        devices = parse_local_list(result.stdout)
        bound = get_bound_devices()

        for d in devices:
            d["bound"] = d["busid"] in bound

        return devices

    except subprocess.CalledProcessError:
        return []


def toggle_bind(device):
    if device["bound"]:
        subprocess.run(["usbip", "unbind", "-b", device["busid"]])
    else:
        subprocess.run(["usbip", "bind", "-b", device["busid"]])


# ============================================================
# CLIENT SIDE
# ============================================================

def parse_remote_list(output):
    """
    Parses:
        2-1: unknown vendor : unknown product (0403:6011)
    """
    devices = []
    lines = output.splitlines()

    device_pattern = re.compile(r"\s*(\S+):\s*(.+?)\s*\((\w+:\w+)\)")

    for line in lines:
        match = device_pattern.search(line)
        if match:
            busid = match.group(1)
            name = match.group(2).strip()
            vidpid = match.group(3)

            devices.append({
                "busid": busid,
                "name": name,
                "vidpid": vidpid,
                "attached": False,
            })

    return devices


def get_attached_devices():
    """
    Returns dict: { remote_busid: local_port }
    Example line:
       5-1 -> usbip://192.168.50.3:3240/2-1
    """
    attached = []

    try:
        result = subprocess.run(
            ["usbip", "port"],
            capture_output=True,
            text=True,
            check=True
        )

        current_port = None
        
        lines = result.stdout.splitlines()
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            # Detect port line
            if line.startswith("Port"):
                m = re.match(r"Port (\d+):", line)
                if m:
                    current_port = m.group(1)
                vendor_line = lines[i+1].strip() if i+1 < len(lines) else ""
                
                bus_id_line = lines[i+2].strip() if i+2 < len(lines) else ""
                bus_id = bus_id_line.split()[0] if bus_id_line else ""
                
                remote_line = lines[i+3].strip() if i+3 < len(lines) else ""
                device = {
                    "busid": bus_id,
                    "port": current_port,
                    "attached": True,
                    "name": vendor_line
                }
                attached.append(device)
                i+=3
            i += 1
                
            


    except subprocess.CalledProcessError:
        pass

    return attached


def get_remote_devices(ip):
    try:
        result = subprocess.run(
            ["usbip", "list", "-r", ip],
            capture_output=True,
            text=True,
            check=True
        )

        devices = parse_remote_list(result.stdout)
        attached = get_attached_devices()

        devices.extend(attached)

        return devices

    except subprocess.CalledProcessError:
        return []


def attach_device(ip, busid):
    subprocess.run(["usbip", "attach", "-r", ip, "-b", busid])


def detach_device(port):
    subprocess.run(["usbip", "detach", "-p", str(port)])


# ============================================================
# UI
# ============================================================

def load_recent_ips():
    if os.path.exists(RECENT_IPS_FILE):
        try:
            with open(RECENT_IPS_FILE, "r") as f:
                data = json.load(f)
                if isinstance(data, list):
                    return data[:MAX_RECENT]
        except Exception:
            pass
    return []

def save_recent_ips(ips):
    try:
        with open(RECENT_IPS_FILE, "w") as f:
            json.dump(ips[:MAX_RECENT], f)
    except Exception:
        pass

def select_ip_menu(stdscr):
    """
    Menu to select an IP:
    - Arrow keys to move
    - Enter to select
    - TAB to input new IP
    - 1-9 (and 0 for 10) to select by number
    """
    curses.curs_set(0)
    stdscr.keypad(True)
    stdscr.clear()
    recent_ips = load_recent_ips()
    selected_idx = 0

    while True:
        stdscr.clear()
        stdscr.addstr(0, 0, "Select IP (TAB to input new IP):", curses.A_BOLD)

        if not recent_ips:
            stdscr.addstr(2, 0, "[No recent IPs saved]")
        else:
            for idx, ip in enumerate(recent_ips):
                prefix = f"{idx + 1}. "
                if idx == selected_idx:
                    stdscr.addstr(idx + 2, 0, prefix + ip, curses.A_REVERSE)
                else:
                    stdscr.addstr(idx + 2, 0, prefix + ip)

        stdscr.refresh()
        key = stdscr.getch()

        # Arrow navigation
        if key == curses.KEY_UP and recent_ips:
            selected_idx = max(0, selected_idx - 1)
        elif key == curses.KEY_DOWN and recent_ips:
            selected_idx = min(len(recent_ips) - 1, selected_idx + 1)

        # Enter selects
        elif key in (curses.KEY_ENTER, 10, 13) and recent_ips:
            ip = recent_ips[selected_idx]
            break

        # TAB for manual input
        elif key == 9:
            curses.curs_set(1)
            curses.echo()
            stdscr.clear()
            stdscr.addstr(0, 0, "Enter new IP: ")
            stdscr.refresh()
            ip = stdscr.getstr(1, 0, 40).decode().strip()
            curses.noecho()
            curses.curs_set(0)
            break

        # Direct selection by number (1-9, 0 for 10)
        elif 49 <= key <= 57:  # keys '1'-'9'
            idx = key - 49  # ord('1') = 49
            if idx < len(recent_ips):
                ip = recent_ips[idx]
                break
        elif key == 48:  # key '0' -> select 10th entry
            idx = 9
            if idx < len(recent_ips):
                ip = recent_ips[idx]
                break

    # Save IP to recent list
    if ip:
        if ip in recent_ips:
            recent_ips.remove(ip)
        recent_ips.insert(0, ip)
        save_recent_ips(recent_ips[:MAX_RECENT])

    return ip

def draw_menu(stdscr, devices, selected_idx, mode, remote_ip):
    stdscr.clear()
    h, w = stdscr.getmaxyx()

    header = "USBIP Manager | TAB=Switch | SPACE=Toggle | R=Refresh | Q=Quit"
    stdscr.addstr(0, 0, header[:w-1], curses.A_BOLD)

    stdscr.addstr(1, 0, f"Mode: {mode.upper()}")

    if mode == "client" and remote_ip:
        stdscr.addstr(2, 0, f"Remote: {remote_ip}")

    if not devices:
        stdscr.addstr(4, 0, "No devices found.")
        stdscr.refresh()
        return

    start_line = 4

    for idx, d in enumerate(devices):

        if mode == "server":
            status = "[x]" if d["bound"] else "[ ]"
        else:
            status = "[x]" if d["attached"] else "[ ]"

        line = f"{status} {d['busid']}  {d['name']}"

        if idx == selected_idx:
            stdscr.addstr(idx + start_line, 0, line[:w-1], curses.A_REVERSE)
        else:
            stdscr.addstr(idx + start_line, 0, line[:w-1])

    stdscr.refresh()


def main(stdscr):
    curses.curs_set(0)
    stdscr.keypad(True)

    mode = "server"
    remote_ip = None
    selected_idx = 0
    requires_refresh = False
    devices = get_local_devices()

    while True:
        if requires_refresh:
            if mode == "server":
                devices = get_local_devices()
            else:
                if not remote_ip:
                    remote_ip = select_ip_menu(stdscr)
                devices = get_remote_devices(remote_ip)
            requires_refresh = False

        if selected_idx >= len(devices):
            selected_idx = 0

        draw_menu(stdscr, devices, selected_idx, mode, remote_ip)

        key = stdscr.getch()

        if key == curses.KEY_UP and selected_idx > 0:
            selected_idx -= 1

        elif key == curses.KEY_DOWN and selected_idx < len(devices) - 1:
            selected_idx += 1

        elif key == ord('\t'):
            mode = "client" if mode == "server" else "server"
            selected_idx = 0
            remote_ip = None
            requires_refresh = True

        elif key in (ord('r'), ord('R')):
            selected_idx = 0
            requires_refresh = True

        elif key == ord(' '):
            if not devices:
                continue

            selected = devices[selected_idx]

            if mode == "server":
                toggle_bind(selected)
            else:
                if selected["attached"]:
                    detach_device(selected["port"])
                else:
                    attach_device(remote_ip, selected["busid"])
                    time.sleep(0.5)
            requires_refresh = True

        elif key in (ord('q'), ord('Q')):
            break

def check_sudo():
    if os.geteuid() != 0:
        print("⚠️  Warning: This script should be run as root (sudo).")
        sys.exit(1)

if __name__ == "__main__":
    check_sudo()
    curses.wrapper(main)
