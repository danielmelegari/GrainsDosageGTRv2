#!/usr/bin/env python3
"""Publisher-only offline RSA license issuer. Requires: pip install cryptography."""
import argparse
import base64
import pathlib
import uuid
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa

parser = argparse.ArgumentParser(description=__doc__)
sub = parser.add_subparsers(dest='command', required=True)
init = sub.add_parser('init', help='Create a NEW publisher key pair; never overwrite a key')
init.add_argument('--directory', type=pathlib.Path, required=True)
issue = sub.add_parser('issue', help='Issue a permanent portable license')
issue.add_argument('--private-key', type=pathlib.Path, required=True)
issue.add_argument('--output', type=pathlib.Path, required=True)
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
    print('Created publisher keys. Keep publisher-private.pem private and backed up.')
else:
    key = serialization.load_pem_private_key(args.private_key.read_bytes(), password=None)
    if not isinstance(key, rsa.RSAPrivateKey) or key.key_size != 2048:
        parser.error('Expected the 2048-bit GrainsDosage publisher RSA key.')
    message = ('GRAINS-DOSAGE-V1:' + uuid.uuid4().hex).encode('ascii')
    signature = key.sign(message, padding.PKCS1v15(), hashes.SHA256())
    serial = 'GDS1.' + base64.b64encode(message).decode() + '.' + base64.b64encode(signature).decode()
    with args.output.open('x') as f:
        f.write(serial + '\n')
    print('License written to', args.output)
