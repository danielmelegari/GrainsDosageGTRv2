#!/usr/bin/env python3
"""Verify the public enrollment matches both platforms; no private keys needed."""
import base64
import hashlib
import json
import pathlib
import re
import struct

root = pathlib.Path(__file__).resolve().parents[1]


def array(path, name):
    text = (root / path).read_text()
    match = re.search(r'\b' + name + r'\[\]\s*=\s*\{([^}]+)\}', text)
    assert match, ('Missing public array', path)
    return bytes(int(item.strip(), 0) for item in match[1].split(','))


def der(tag, content):
    size = len(content)
    length = bytes([size]) if size < 128 else bytes([0x80 + (size.bit_length() + 7) // 8]) + size.to_bytes((size.bit_length() + 7) // 8, 'big')
    return bytes([tag]) + length + content


def from_cng(blob):
    assert len(blob) == 283
    assert struct.unpack('<6I', blob[:24]) == (0x31415352, 2048, 3, 256, 0, 0)
    assert blob[24:27] == b'\x01\x00\x01' and blob[27] & 0x80 and blob[-1] & 1
    return der(0x30, der(2, b'\0' + blob[27:]) + der(2, blob[24:27]))


request = json.loads((root / 'config/publisher-public-key.json').read_text())
assert set(request) == {'format', 'algorithm', 'public_key_pkcs1_der_base64', 'sha256'}
assert request['format'] == 'GrainsDosage-Public-Key-v1'
assert request['algorithm'] == 'RSA-2048-SHA256-PKCS1-v1_5'
public = base64.b64decode(request['public_key_pkcs1_der_base64'], validate=True)
assert hashlib.sha256(public).hexdigest() == request['sha256']
assert array('src/license_public_key.h', 'publicKey') == public
assert from_cng(base64.b64decode((root / 'config/windows-trial-public.b64').read_text().strip(), validate=True)) == public
previous = array('src/license_legacy_public_key.h', 'legacyPublicKey')
assert from_cng(array('src/trial_legacy_public_key.h', 'legacyTrialPublicKey')) == previous
assert previous != public
print('PASS: enrollment, macOS/generator and Windows share the same RSA public key; previous verification key retained')
