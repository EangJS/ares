import argparse
import subprocess
from relay import USBRelay, download_mode, reset
import os
import shutil
import glob
from get_artifacts import download_artifacts_from_github
import serial
import time
from dotenv import load_dotenv
from utils import *

load_dotenv()

TIZENRT_DIR = "/tmp/build/TizenRT"
OS_DIR = f"{TIZENRT_DIR}/os"
BAUD = 1500000

def run(cmd, cwd=None):
    print(f"\n>>> {cmd}")
    result = subprocess.run(cmd, shell=True, check=True, cwd=cwd)
    print(f">>> Command finished with return code {result.returncode}")

def copy_files(src_pattern, dest_dir):
    os.makedirs(dest_dir, exist_ok=True)
    run(f"cp -r {src_pattern} {dest_dir}")

def flash_board(port):
    relay = USBRelay()
    print("Putting device into download mode...")
    download_mode(relay)
    print("Downloading firmware...")
    try:
        run(f"sudo make download all port={args.port}", cwd=OS_DIR)
    except subprocess.CalledProcessError as e:
        pass
    print("Download complete. Resetting device...")
    reset(relay)
    relay.close()
    print("Device reset complete.")

def run_ares(port):
    ser = serial.Serial("/dev/" + port, BAUD, timeout=1)
    time.sleep(5)
    ser.write(b'x\n')
    time.sleep(1)
    ser.write(b"ares\n")
    while True:
        line = ser.readline()
        if line:
            print(line)
        time.sleep(0.1)

def main(args):
    run("sudo rm -rf /tmp/build")
    if args.download_artifacts:
        clone_repos(TIZENRT_DIR, f"{TIZENRT_DIR}/apps/examples/ares", clone_ares=False)
        download_artifacts_from_github(
            repo="EangJS/ares",
            workflow_file="ci.yml",
            dest_dir=f"{TIZENRT_DIR}/build/output",
            github_token=os.getenv("GITHUB_PAT"),
        )
        if args.config:
            copy_files(f"{TIZENRT_DIR}/build/output/ares/assets/{args.config}/bin/*", f"{TIZENRT_DIR}/build/output/bin")
            run(f"cp {TIZENRT_DIR}/build/output/ares/assets/{args.config}/.bininfo {TIZENRT_DIR}/os")
    elif args.build:
        clone_repos(TIZENRT_DIR, f"{TIZENRT_DIR}/apps/examples/ares", clone_ares=True)
        local_build(build_dir=f"{TIZENRT_DIR}/build", tizenrt_dir=TIZENRT_DIR, ares_dir=f"{TIZENRT_DIR}/apps/examples/ares", config=args.config)
    else:
        clone_repos(TIZENRT_DIR, f"{TIZENRT_DIR}/apps/examples/ares", clone_ares=False)

    flash_board(args.port)
    run_ares(args.port)

# example command: python3 autodownload.py --port ttyUSB0 --download-artifacts True --config ares_ddr

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Asuto-download script")
    port = parser.add_argument(
        "--port",
        required=True,
        help="Target Port to download to",
    )
    download_artifacts = parser.add_argument(
        "--download-artifacts",
        required=False,
        help="True or False to download artifacts from GitHub",
    )
    config = parser.add_argument(
        "--config",
        required=False,
        help="Build configuration to use",
    )
    build = parser.add_argument(
        "--build",
        required=False,
        help="True or False to perform local build before downloading",
    )
    args = parser.parse_args()
    if (args.build == "true" and not args.config) or (args.download_artifacts == "true" and not args.config):
        parser.error("--build and --download-artifacts requires --config to be set")
    main(args)
