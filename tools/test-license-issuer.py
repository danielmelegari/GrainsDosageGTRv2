#!/usr/bin/env python3
"""Isolated publisher tests; never reads production keys or ledgers."""
import base64
import concurrent.futures
import pathlib
import sqlite3
import subprocess
import sys
import tempfile
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding

issuer = pathlib.Path(__file__).with_name('license-issuer.py')
with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    def call(*args, ok=True):
        r = subprocess.run([sys.executable, str(issuer), *map(str, args)], capture_output=True, text=True)
        assert (r.returncode == 0) == ok, r.stdout + r.stderr
        return r
    call('init', '--directory', root)
    call('init', '--directory', root, ok=False)
    key = root / 'publisher-private.pem'
    def command(cmd, *args, **kwargs):
        return call(cmd, '--private-key', key, *args, **kwargs)
    license_id = command('create', '--customer', 'Customer One').stdout.strip()
    other_id = command('create', '--customer', 'Customer Two').stdout.strip()
    assert license_id != other_id
    def activate(number, output, ok=True, license=license_id):
        return command('activate', '--license', license, '--device', 'GDDEV2-' + format(number, '064x'), '--output', root / output, ok=ok)
    for i in range(1, 4):
        activate(i, str(i))
    activate(4, 'fourth', ok=False)
    assert not (root / 'fourth').exists()
    activate(1, 'reissue')
    assert (root / '1').read_text() == (root / 'reissue').read_text()
    activate(1, 'reissue', ok=False)
    assert '3/3' in command('list', '--license', license_id).stdout
    activate(4, 'other-customer', license=other_id)
    public = serialization.load_der_public_key((root / 'publisher-public.der').read_bytes())
    fields = (root / '1').read_text().strip().split('.')
    assert fields[0] == 'GDS2'
    payload, signature = map(base64.b64decode, fields[1:])
    assert payload.decode() == 'GRAINS-DOSAGE-V2:' + license_id[5:] + ':' + format(1, '064x') + ':3'
    public.verify(signature, payload, padding.PKCS1v15(), hashes.SHA256())
    # Eight concurrent requests cannot race past the three-device ceiling.
    race_id = command('create', '--customer', 'Concurrent').stdout.strip()
    def racing(i):
        return subprocess.run([sys.executable, str(issuer), 'activate', '--private-key', str(key), '--license', race_id,
                               '--device', 'GDDEV2-' + format(i, '064x'), '--output', str(root / ('race'+str(i)))], capture_output=True).returncode
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        codes = list(pool.map(racing, range(8)))
    assert codes.count(0) == 3, codes
    assert '3/3' in command('list', '--license', race_id).stdout
    # A failed export reserves its slot safely; retrying does not consume another.
    retry_id = command('create', '--customer', 'Retry').stdout.strip()
    activate(1, 'missing/fail', ok=False, license=retry_id)
    assert '1/3' in command('list', '--license', retry_id).stdout
    activate(1, 'retry', license=retry_id)
    assert '1/3' in command('list', '--license', retry_id).stdout
    command('activate', '--license', license_id, '--device', 'bad', '--output', root/'bad', ok=False)
    command('activate', '--license', 'GDL2-'+'f'*32, '--device', 'GDDEV2-'+'f'*64, '--output', root/'unknown', ok=False)
    call('init', '--directory', root/'wrong-key')
    call('list', '--private-key', root/'wrong-key/publisher-private.pem', '--database', root/'licenses.sqlite', '--license', license_id, ok=False)
    (root/'licenses.sqlite').rename(root/'backup.sqlite')
    command('list', '--license', license_id, ok=False)
    assert not (root/'licenses.sqlite').exists()
print('PASS: signed device binding, three-device cap, independent customers, repeat activation, concurrent issuance, export recovery, wrong key and missing ledger')
