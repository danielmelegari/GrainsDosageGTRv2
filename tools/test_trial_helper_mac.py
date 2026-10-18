#!/usr/bin/env python3
"""Check the real Keychain through helper copies in two host bundle paths."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import uuid

identifier = uuid.uuid4().hex
env = dict(os.environ, GRAINS_TRIAL_TEST_ID=identifier)
service = 'com.grainsdosage.auto-test.' + identifier
start = int(time.time())
root = Path(tempfile.gettempdir()) / ('grains-auto-trial-test-' + identifier)
try:
    with tempfile.TemporaryDirectory() as directory:
        helpers = []
        for host in ('VST3', 'AudioUnit'):
            path = Path(directory) / host / 'GrainsDosageTrialHelper'
            path.parent.mkdir()
            shutil.copy2(sys.argv[1], path)
            helpers.append(path)
        def check(helper, create, now, expected):
            result = subprocess.run([str(helper), str(int(create)), str(now), '0', '0', '0'],
                                    env=env, capture_output=True, text=True, timeout=5, check=True)
            state, first, last = map(int, result.stdout.split())
            assert state == expected, (result.stdout, result.stderr)
            if expected != 0:
                assert first == start
            return first, last
        check(helpers[0], False, start, 0)  # Scanning does not start a trial.
        check(helpers[0], True, start, 1)
        check(helpers[1], True, start + 1, 1)  # Other host must not ask for authorization.
        check(helpers[0], True, start + 14*86400, 2)
        check(helpers[1], True, start + 14*86400, 2)
        check(helpers[1], True, start, 2)  # Restart/rollback never extends the trial.
    print('PASS: isolated Keychain helper works across bundle paths and preserves expiry')
finally:
    subprocess.run(['/usr/bin/security', 'delete-generic-password', '-s', service, '-a', 'first-use'],
                   capture_output=True, timeout=5, check=False)
    shutil.rmtree(root, ignore_errors=True)
