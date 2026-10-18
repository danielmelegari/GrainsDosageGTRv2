#!/usr/bin/env python3
"""Generate a macOS trial request, or install a publisher-signed trial license."""
import argparse
import hashlib
import json
import pathlib
import re
import shutil
import subprocess

def machine_id():
    data = subprocess.check_output(["ioreg", "-rd1", "-c", "IOPlatformExpertDevice"], text=True)
    found = re.search(r'"IOPlatformUUID"\\s*=\\s*"([^"]+)"', data)
    if not found:
        raise SystemExit("Could not read the macOS machine UUID")
    return hashlib.sha256(("GrainsDosage-mac-machine-v1:" + found.group(1).lower()).encode()).hexdigest()

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("action", choices=("request", "install"))
p.add_argument("license", nargs="?", type=pathlib.Path)
args = p.parse_args()
if args.action == "request":
    target = pathlib.Path.home() / "Desktop/GrainsDosage-machine-request.json"
    target.write_text(json.dumps({"product": "GrainsDosage-Trial-v1", "machine_id": machine_id()}, indent=2))
    print("Send this file to the publisher:", target)
else:
    if not args.license or not args.license.is_file():
        p.error("install requires the issued trial.license file")
    contents = args.license.read_text()
    lines = contents.splitlines()
    if len(lines) != 5 or lines[0] != "GRAINS-TRIAL-1" or lines[1] != machine_id():
        raise SystemExit("License format or machine ID mismatch")
    target = pathlib.Path.home() / "Library/Application Support/GrainsDosage-Trial"
    target.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.license, target / "trial.license")
    (target / "trial.license").chmod(0o600)
    print("License installed. Reopen your DAW to activate. The plugin verifies the signature and expiry.")
