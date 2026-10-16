"""Compare process CPU time and exact output hashes; run on the same machine."""
import re
import statistics
import subprocess
import sys
from pathlib import Path

allow_audio_change = "--allow-audio-change" in sys.argv[4:]
results = {}
for repeat in range(3):
    for label, exe in zip(('before', 'after'), sys.argv[1:3]):
        output = subprocess.check_output([exe], text=True)
        for line in output.splitlines():
            match = re.fullmatch(r'preset=(-?\d+) cpu_one_core_pct=([\d.]+) audio_hash=(\d+)', line)
            if not match:
                raise ValueError(f'Unexpected benchmark output: {line}')
            preset, cpu, audio_hash = match.groups()
            results.setdefault((label, preset), []).append((float(cpu), audio_hash))
lines = ['48 kHz stereo, 256-sample blocks, 10 seconds/preset; median of 3 runs.',
         'CPU is % of one core for the engine only, not the DAW or GUI.',
         'preset | before | after | reduction | audio']
for preset in ('-1', '0', '1', '8', '24', '25', '29', '100'):
    before, after = results['before', preset], results['after', preset]
    for group in (before, after):
        if len({h for _, h in group}) != 1:
            raise ValueError(f'Non-deterministic audio for preset {preset}')
    identical = before[0][1] == after[0][1]
    if not identical and not allow_audio_change:
        raise ValueError(f'Audio output changed for preset {preset}')
    audio = 'identical' if identical else 'changed (quality / grain continuity fixes)'
    a, b = (statistics.median(c for c, _ in group) for group in (before, after))
    lines.append(f'{preset} | {a:.3f}% | {b:.3f}% | {100*(1-b/a):.1f}% | {audio}')
report = '\n'.join(lines) + '\n'
Path(sys.argv[3]).write_text(report)
print(report)
