#!/usr/bin/env python3
"""Package host-test VST3 bundle without installers or production publishing."""
import pathlib
import sys
import zipfile

root = pathlib.Path(sys.argv[1]).resolve()
out = pathlib.Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
plugins = [p for p in root.rglob("GrainsDosage.vst3") if p.is_dir()]
if len(plugins) != 1:
    sys.exit("Cannot locate a unique built GrainsDosage.vst3")
plugin = plugins[0]
archive = out / "GrainsDosage-host-compat.vst3.zip"
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
    for path in plugin.rglob("*"):
        if path.is_file():
            z.write(path, pathlib.Path("GrainsDosage.vst3") / path.relative_to(plugin))
print(archive)
