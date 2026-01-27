import requests
import os
from zipfile import ZipFile
import tarfile

def download_artifacts_from_github(repo, workflow_file=None, dest_dir="./artifacts", github_token=None):
    headers = {}
    if github_token:
        headers["Authorization"] = f"token {github_token}"

    # Step 1: Get workflows
    workflows_url = f"https://api.github.com/repos/{repo}/actions/workflows"
    response = requests.get(workflows_url, headers=headers)
    response.raise_for_status()
    workflows = response.json()["workflows"]
    if not workflows:
        raise Exception("No workflows found.")

    workflow = None
    if workflow_file:
        workflow = next((w for w in workflows if w["path"].endswith(workflow_file)), None)
        if not workflow:
            raise Exception(f"Workflow file '{workflow_file}' not found.")
    else:
        workflow = workflows[0]

    workflow_id = workflow["id"]
    print(f"Using workflow: {workflow['name']} (ID: {workflow_id})")

    # Step 2: Get latest workflow run
    runs_url = f"https://api.github.com/repos/{repo}/actions/workflows/{workflow_id}/runs"
    response = requests.get(runs_url, headers=headers)
    response.raise_for_status()
    runs = response.json()["workflow_runs"]
    if not runs:
        raise Exception("No workflow runs found.")
    latest_run = runs[0]
    print(f"Latest run ID: {latest_run['id']} (Status: {latest_run['status']})")

    # Step 3: Get artifacts
    artifacts_url = f"https://api.github.com/repos/{repo}/actions/runs/{latest_run['id']}/artifacts"
    response = requests.get(artifacts_url, headers=headers)
    response.raise_for_status()
    artifacts = response.json()["artifacts"]
    if not artifacts:
        raise Exception("No artifacts found.")

    os.makedirs(dest_dir, exist_ok=True)

    # Step 4: Download to drive and extract
    for artifact in artifacts:
        zip_file_path = os.path.join(dest_dir, f"{artifact['name']}.zip")
        os.makedirs(os.path.dirname(zip_file_path), exist_ok=True)

        download_url = artifact["archive_download_url"]
        print(f"Downloading artifact '{artifact['name']}' to '{zip_file_path}'...")

        # Stream download to file
        with requests.get(download_url, headers=headers, stream=True) as r:
            r.raise_for_status()
            with open(zip_file_path, "wb") as f:
                progress = 0
                for chunk in r.iter_content(chunk_size=8192):
                    f.write(chunk)
                    progress += len(chunk)
                    print(f"\rDownloaded {progress / (1024 * 1024):.2f} MB", end="")

        if not os.path.exists(zip_file_path):
            raise FileNotFoundError(f"Failed to download zip: {zip_file_path}")
        print(f"Downloaded '{zip_file_path}' successfully.")

        # Extract zip
        extract_path = os.path.join(dest_dir, artifact["name"])
        os.makedirs(extract_path, exist_ok=True)
        with ZipFile(zip_file_path, "r") as zip_ref:
            zip_ref.extractall(extract_path)
        print(f"Extracted zip to '{extract_path}'")

        # Extract tar.xz inside zip
        for file_name in os.listdir(extract_path):
            if file_name.endswith(".tar.xz"):
                file_name_only = file_name[:-7]  # Remove .tar.xz
                tar_path = os.path.join(extract_path, file_name)
                print(f"Extracting tar.xz: {tar_path}")
                with tarfile.open(tar_path, "r:xz") as tar:
                    tar.extractall(dest_dir + f"/{file_name_only}")
                print(f"Extracted tar.xz to '{dest_dir + f"/{file_name_only}"}'")

