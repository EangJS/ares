import os
import sys
import subprocess
import tarfile
import glob

TIZENRT_REPO = "https://github.com/Samsung/TizenRT.git"
ARES_REPO = "https://github.com/EangJS/ares.git"
BOARD = "rtl8730e"
ARTIFACTS = ["km4_boot_all.bin", "*.trpk", "bootparam.bin", "target_img2.axf"]

def apply_patches(patch_dir, ares_dir, tizen_dir):
    patch_files = [
        f for f in os.listdir(patch_dir) if f.endswith('.patch')
    ]
    for patch_file in patch_files:
        patch_path = os.path.join(patch_dir, patch_file)
        if "tizen" in patch_file.lower():
            run(f"git apply {patch_path}", cwd=tizen_dir)
        elif "ares" in patch_file.lower():
            run(f"git apply {patch_path}", cwd=ares_dir)

def clone(repo_url, target_dir, requires_submodule=False):
    run(f"git clone {repo_url} {target_dir}")
    if requires_submodule:
        run("git submodule update --init --recursive", cwd=target_dir)

def clone_repos(tizenrt_dir, ares_dir, clone_ares=False):
    os_dir = os.path.join(tizenrt_dir, "os")
    patch_dir = os.path.join(ares_dir, "scripts", "patches")
    requires_patching = False
    if not os.path.isdir(tizenrt_dir):
        requires_patching = True
        clone(TIZENRT_REPO, tizenrt_dir)
    else:
        print("TizenRT already cloned, skipping clone.")

    if clone_ares:
        if not os.path.isdir(ares_dir):
            clone(ARES_REPO, ares_dir, requires_submodule=True)
        else:
            print("Ares already cloned, skipping clone.")
    elif not os.path.isdir(ares_dir):
        run(f"mkdir -p {ares_dir}")
        run(f"cp -r . {ares_dir}/", cwd="..")

    if requires_patching:
        apply_patches(patch_dir, ares_dir, tizenrt_dir)

    if not os.path.isdir(os_dir):
        print("ERROR: os directory not found. Repo layout unexpected.")
        sys.exit(1)

def run(cmd, cwd=None):
    print(f"\n>>> {cmd}")
    result = subprocess.run(cmd, shell=True, check=True, cwd=cwd)
    print(f">>> Command finished with return code {result.returncode}")

def compress_directory(src_dir, tar_path):
    print(f"Compressing {src_dir} -> {tar_path}")
    with tarfile.open(tar_path, "w:xz") as tar:
        tar.add(src_dir, arcname=os.path.basename(src_dir))

def verify_build(tizenrt_dir):
    bin_dir = os.path.join(tizenrt_dir, "build", "output", "bin")
    for artifact in ARTIFACTS:
        pattern = os.path.join(bin_dir, artifact)
        matches = glob.glob(pattern)

        if not matches:
            print(f"ERROR: Build output missing essential artifact: {artifact}")
            sys.exit(1)

def cleanup_artifacts(tizenrt_dir):
    bin_dir = os.path.join(tizenrt_dir, "build", "output", "bin")

    allowed_files = set()
    for artifact in ARTIFACTS:
        pattern = os.path.join(bin_dir, artifact)
        allowed_files.update(
            os.path.basename(p) for p in glob.glob(pattern)
        )

    for filename in os.listdir(bin_dir):
        file_path = os.path.join(bin_dir, filename)

        if os.path.isfile(file_path) and filename not in allowed_files:
            os.remove(file_path)

def local_build(build_dir, tizenrt_dir, ares_dir, config):
    os_dir = os.path.join(tizenrt_dir, "os")
    run("chmod +x tools/configure.sh", cwd=os_dir)

    build_config = f"{BOARD}/{config}"
    print(f"\n=== Building configuration: {build_config} ===")

    run(f"cp -r {config} {tizenrt_dir}/build/configs/{BOARD}/")
    try:
        run(f"./dbuild.sh distclean", cwd=os_dir)
        run(f"./tools/configure.sh {build_config}", cwd=os_dir)
    except subprocess.CalledProcessError:
        pass

    run("./dbuild.sh", cwd=os_dir)
    verify_build(tizenrt_dir)
    cleanup_artifacts(tizenrt_dir)
    print("\nBuild complete!")
    run(f"ls -l build/output/bin", cwd=tizenrt_dir)
    run(f"mkdir -p {build_dir}/assets/{config}")
    run(f"cp os/.bininfo {build_dir}/assets/{config}/", cwd=tizenrt_dir)
    run(f"cp -r build/output/bin {build_dir}/assets/{config}/", cwd=tizenrt_dir)

