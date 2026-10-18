#!/usr/bin/env python3
"""Bound AU validation and collect stacks before killing a stalled validator."""
import argparse
import os
from pathlib import Path
import signal
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--diagnostics', required=True)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not command:
        parser.error('missing validator command')
    process = subprocess.Popen(command, start_new_session=True)
    try:
        return process.wait(timeout=args.timeout)
    except subprocess.TimeoutExpired:
        print(f'::error::AU validation exceeded {args.timeout:g} seconds; collecting diagnostics', flush=True)
        root = Path(args.diagnostics)
        root.mkdir(parents=True, exist_ok=True)
        try:
            snapshot = subprocess.check_output(['ps', '-axo', 'pid,ppid,command'], text=True, timeout=5)
            (root / 'processes.txt').write_text(snapshot)
            children = {process.pid}
            rows = [line.split(None, 2) for line in snapshot.splitlines()[1:]]
            for _ in range(10):
                previous = len(children)
                children.update(int(row[0]) for row in rows if len(row) == 3 and int(row[1]) in children)
                if len(children) == previous:
                    break
            if sys.platform == 'darwin':
                for pid in sorted(children):
                    subprocess.run(['/usr/bin/sample', str(pid), '2', '-file', str(root / f'stack-{pid}.txt')], timeout=10, check=False)
        except (OSError, subprocess.SubprocessError) as error:
            print(f'Diagnostic collection: {error}', flush=True)
        finally:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.wait()
        return 124


if __name__ == '__main__':
    sys.exit(main())
