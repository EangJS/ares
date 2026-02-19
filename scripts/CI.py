#!/usr/bin/env python3
# This script should be executed from the 'ares/scripts' directory.

import os
import subprocess
import tarfile
import re
import requests
import argparse
import sys
import glob
from automator.utils import *

BUILD_DIR = "/tmp/build"
TIZENRT_DIR = BUILD_DIR + "/TizenRT"
CONFIGS = ["ares_ddr_st7785", "ares_psram"]

def get_next_build_number(repo_url, repo_name, auth):
    """
    Get next build number by listing existing assets in the raw repo.
    """
    builds = set()
    continuation_token = None

    while True:
        params = {"repository": repo_name}
        if continuation_token:
            params["continuationToken"] = continuation_token

        url = f"{repo_url}/service/rest/v1/assets"
        resp = requests.get(url, params=params, auth=auth)
        resp.raise_for_status()
        data = resp.json()

        for item in data.get("items", []):
            path = item.get("path", "")
            m = re.match(rf"{repo_name}/build_(\d+)/", path)
            if m:
                builds.add(int(m.group(1)))

        continuation_token = data.get("continuationToken")
        if not continuation_token:
            break

    return max(builds) + 1 if builds else 1


def upload_to_nexus_raw(repo_url, repo_name, file_path, directory, auth=None):
    """
    Upload a file to Nexus Raw repo with authentication
    """
    filename = os.path.basename(file_path)
    url = f"{repo_url}/service/rest/v1/components?repository={repo_name}"

    files = {
        "raw.asset1": (filename, open(file_path, "rb"), "application/zip")
    }

    data = {
        "raw.directory": directory,
        "raw.asset1.filename": filename  # THIS IS REQUIRED
    }

    response = requests.post(url, data=data, files=files, auth=auth)

    if response.status_code not in (200, 201, 204):
        raise RuntimeError(f"Upload failed: {response.status_code} {response.text}")

    print(f"Uploaded {file_path} to {directory} successfully")

# =========================
# Main logic
# =========================
def main(args):
    for config in CONFIGS:
        ares_dir = os.path.join(TIZENRT_DIR, "apps", "examples", "ares")
        clone_repos(TIZENRT_DIR, ares_dir, clone_ares=False)
        local_build(build_dir=BUILD_DIR, tizenrt_dir=TIZENRT_DIR, ares_dir=ares_dir, config=config)
    assets_zip_path = f"{BUILD_DIR}/artifacts/ares.tar.xz"
    run(f"mkdir -p {BUILD_DIR}/artifacts")
    compress_directory(f"{BUILD_DIR}/assets", assets_zip_path)
    if args.upload_url:
        auth=(
                args.username,
                args.password
            ) if args.username and args.password else None
        build_number = get_next_build_number(args.upload_url, "Ares", auth)
        upload_to_nexus_raw(
            repo_url=args.upload_url,
            repo_name="Ares",
            file_path=assets_zip_path,
            directory=f"build_{build_number}",
            auth=auth,
        )

    run(f"sudo rm -rf {BUILD_DIR}/TizenRT")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Clone, build, zip, and upload TizenRT")
    parser.add_argument(
        "--upload-url",
        required=False,
        help="Destination URL to upload the zip file (HTTP POST)",
    )
    parser.add_argument(
        "--username",
        required=False,
        help="Username for authentication (if required)",
    )
    parser.add_argument(
        "--password",
        required=False,
        help="Password for authentication (if required)",
    )

    args = parser.parse_args()
    main(args)
