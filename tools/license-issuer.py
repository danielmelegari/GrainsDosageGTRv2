#!/usr/bin/env python3
"""Publisher-only offline issuer: one license, at most three Mac device activations.
Requires Python 3 and cryptography. Back up the private key AND the SQLite ledger.
"""
import argparse
import base64
import hashlib
import pathlib
import re
import sqlite3
import uuid
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    init = sub.add_parser('init', help='Create a NEW publisher key pair; never overwrite a key')
    init.add_argument('--directory', type=pathlib.Path, required=True)
    for command in ('create', 'activate', 'list'):
        p = sub.add_parser(command)
        p.add_argument('--private-key', type=pathlib.Path, required=True)
        p.add_argument('--database', type=pathlib.Path, help='Default: licenses.sqlite beside the private key')
        if command == 'create':
            p.add_argument('--customer', required=True, help='Publisher-only customer reference')
        else:
            p.add_argument('--license', required=True, help='GDL2 license ID from create')
        if command == 'activate':
            p.add_argument('--device', required=True, help='GDDEV2 code copied from the customer Mac')
            p.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    if args.command == 'init':
        args.directory.mkdir(parents=True, exist_ok=True, mode=0o700)
        private = args.directory / 'publisher-private.pem'
        public = args.directory / 'publisher-public.der'
        if private.exists() or public.exists():
            parser.error('Key files already exist; preserve your existing publisher key.')
        key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
        with private.open('xb') as f:
            private.chmod(0o600)
            f.write(key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
        public.write_bytes(key.public_key().public_bytes(serialization.Encoding.DER, serialization.PublicFormat.PKCS1))
        print('Created publisher keys. Back up the private key and activation ledger.')
        return
    key = serialization.load_pem_private_key(args.private_key.read_bytes(), password=None)
    if not isinstance(key, rsa.RSAPrivateKey) or key.key_size != 2048:
        parser.error('Expected the 2048-bit GrainsDosage publisher RSA key.')
    fingerprint = hashlib.sha256(key.public_key().public_bytes(serialization.Encoding.DER, serialization.PublicFormat.PKCS1)).hexdigest()
    database = args.database or args.private_key.parent / 'licenses.sqlite'
    if args.command != 'create' and not database.is_file():
        parser.error('Ledger missing. Restore your original licenses.sqlite backup; do not create a replacement.')
    if args.command != 'create' and not re.fullmatch(r'GDL2-[0-9a-f]{32}', args.license):
        parser.error('Invalid license ID.')
    if args.command == 'activate':
        if not re.fullmatch(r'GDDEV2-[0-9a-f]{64}', args.device):
            parser.error('Invalid device code. Copy the complete code from ACTIVATE on the customer Mac.')
        if args.output.exists():
            parser.error('Output already exists; choose a new filename.')
    # BEGIN IMMEDIATE serializes simultaneous requests, including the fourth device.
    with sqlite3.connect(str(database), timeout=30) as db:
        database.chmod(0o600)
        db.execute('BEGIN IMMEDIATE')
        db.execute('CREATE TABLE IF NOT EXISTS publisher (fingerprint TEXT PRIMARY KEY)')
        db.execute('CREATE TABLE IF NOT EXISTS licenses (id TEXT PRIMARY KEY, customer TEXT NOT NULL)')
        db.execute('CREATE TABLE IF NOT EXISTS activations (license TEXT NOT NULL, device TEXT NOT NULL, PRIMARY KEY(license,device))')
        saved = db.execute('SELECT fingerprint FROM publisher').fetchall()
        if saved and saved != [(fingerprint,)]:
            parser.error('This ledger belongs to another publisher key.')
        db.execute('INSERT OR IGNORE INTO publisher VALUES (?)', (fingerprint,))
        if args.command == 'create':
            license_id = 'GDL2-' + uuid.uuid4().hex
            db.execute('INSERT INTO licenses VALUES (?,?)', (license_id, args.customer))
            print(license_id)
            return
        if not db.execute('SELECT 1 FROM licenses WHERE id=?', (args.license,)).fetchone():
            parser.error('Unknown license. Use its original ledger; do not recreate it.')
        devices = [row[0] for row in db.execute('SELECT device FROM activations WHERE license=? ORDER BY device', (args.license,))]
        if args.command == 'list':
            print('%s: %d/3 devices' % (args.license, len(devices)))
            for device in devices:
                print(device)
            return
        if args.device not in devices:
            if len(devices) >= 3:
                parser.error('Limit reached: this license already has 3 devices. Offline activations cannot be revoked.')
            db.execute('INSERT INTO activations VALUES (?,?)', (args.license, args.device))
        message = ('GRAINS-DOSAGE-V2:' + args.license[5:] + ':' + args.device[7:] + ':3').encode('ascii')
        signature = key.sign(message, padding.PKCS1v15(), hashes.SHA256())
        serial = 'GDS2.' + base64.b64encode(message).decode() + '.' + base64.b64encode(signature).decode()
    # Commit the slot BEFORE publishing a usable signature. A failed file write
    # can be retried for this same device without consuming another slot.
    with args.output.open('x') as f:
        f.write(serial + '\n')
    print('Activation written to', args.output)


if __name__ == '__main__':
    main()
