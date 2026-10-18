"""Usage: python compare_granulizer_reslice.py BEFORE_EXE AFTER_EXE
Build benchmark_granulizer_reslice.cpp against each source tree with identical
compiler/options, then run on the same otherwise idle machine. No timing gate:
wall-time peaks depend on the scheduler. Differing audio hashes fail the run.
"""
import statistics
import subprocess
import sys

results = {"before": [], "after": []}
for repeat in range(3):
    for name in (["before", "after"] if repeat % 2 == 0 else ["after", "before"]):
        exe = sys.argv[1 if name == "before" else 2]
        rows = [line.split() for line in subprocess.check_output([exe], text=True).splitlines()]
        if len(rows) != 5 or any(len(row) != 8 for row in rows):
            raise ValueError("Unexpected benchmark output")
        results[name].append(rows)
print("case | sample rate | block | before CPU % | after CPU % | reduction % | before/after p99 us | audio")
for i in range(5):
    before, after = ([run[i] for run in results[name]] for name in ("before", "after"))
    if len({tuple(row[:3]) for row in before + after}) != 1:
        raise ValueError("Benchmark configurations differ")
    if any(float(row[7]) <= 0 for row in before + after):
        raise ValueError(f"Unexpected silent render in case {i}")
    if len({row[6] for row in before + after}) != 1:
        raise ValueError(f"Audio hash changed in case {i}")
    a, b = (statistics.median(float(row[3]) for row in rows) for rows in (before, after))
    ap, bp = (statistics.median(float(row[4]) for row in rows) for rows in (before, after))
    print(f"{i} | {before[0][1]} | {before[0][2]} | {a:.3f} | {b:.3f} | {100*(1-b/a):.1f} | {ap:.2f}/{bp:.2f} | identical hashes")
