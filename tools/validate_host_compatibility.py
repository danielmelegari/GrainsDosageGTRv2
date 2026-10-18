#!/usr/bin/env python3
"""Find and run Steinberg's VST3 validator against the freshly built bundle."""
import pathlib
import subprocess
import sys

root = pathlib.Path(sys.argv[1]).resolve()
plugins = [p for p in root.rglob("GrainsDosage.vst3") if p.is_dir()]
validators = [p for p in root.rglob("*") if p.is_file() and
              p.name.lower() in ("validator", "validator.exe")]
if len(plugins) != 1:
    sys.exit(f"Expected exactly one GrainsDosage.vst3 bundle, got {plugins}")
if not validators:
    sys.exit("Steinberg validator executable not found in build output")
validator = next((p for p in validators if "Release" in p.parts), validators[0])
print(f"Validating {plugins[0]} using {validator}", flush=True)
sys.exit(subprocess.run([str(validator), "-e", str(plugins[0])], check=False).returncode)
