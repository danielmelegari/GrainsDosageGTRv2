#!/usr/bin/env python3
"""Offline issuer. Keep the private key on the publisher's computer only."""
import argparse
import datetime as dt
import json
import re
import struct
from pathlib import Path

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa

EPOCH = dt.datetime(1601, 1, 1, tzinfo=dt.timezone.utc)
DURATION = 14 * 24 * 60 * 60 * 10_000_000


def ticks(value):
    delta = value.astimezone(dt.timezone.utc) - EPOCH
    return (delta.days * 86400 + delta.seconds) * 10_000_000 + delta.microseconds * 10


def public_blob(key):
    numbers = key.public_key().public_numbers()
    exponent = numbers.e.to_bytes((numbers.e.bit_length() + 7) // 8, "big")
    modulus = numbers.n.to_bytes(256, "big")
    return struct.pack("<6I", 0x31415352, 2048, len(exponent), len(modulus), 0, 0) + exponent + modulus


def keygen(key_path, header_path):
    if key_path.exists() or header_path.exists() or header_path.with_suffix(".b64").exists():
        raise ValueError("Refusing to replace an existing key or public header")
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    encoded = key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8,
                               serialization.NoEncryption())
    # Exclusive creation and restrictive permissions before writing private bytes.
    import os
    fd = os.open(key_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, "wb") as stream:
        stream.write(encoded)
    blob = public_blob(key)
    import base64
    header_path.with_suffix(".b64").write_text(base64.b64encode(blob).decode("ascii"), encoding="ascii")
    header_path.write_text("#pragma once\n// Public verification key only; private key is never shipped.\n"
        "namespace grains::trial {\ninline constexpr unsigned char trialPublicKey[] = {\n  " +
        ", ".join(f"0x{b:02x}" for b in blob) + "\n};\n}\n", encoding="utf-8")


def issue(key_path, machine_id, start):
    if not re.fullmatch(r"[0-9a-f]{64}", machine_id):
        raise ValueError("Machine ID must contain exactly 64 lowercase hexadecimal characters")
    if start.tzinfo is None:
        raise ValueError("Start date must include a timezone, e.g. 2026-10-12T12:00:00Z")
    begin = ticks(start)
    if begin <= 0 or begin + DURATION >= 2**64:
        raise ValueError("Start date is outside the supported range")
    key = serialization.load_pem_private_key(key_path.read_bytes(), password=None)
    if not isinstance(key, rsa.RSAPrivateKey) or key.key_size != 2048:
        raise ValueError("Expected the original RSA-2048 issuing key")
    payload = f"GRAINS-TRIAL-1\n{machine_id}\n{begin}\n{begin + DURATION}\n".encode("ascii")
    signature = key.sign(payload, padding.PKCS1v15(), hashes.SHA256())
    return payload + signature.hex().encode("ascii") + b"\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    generate = commands.add_parser("keygen", help="One-time setup; replacing the key requires rebuilding the plugin")
    generate.add_argument("--key", type=Path, required=True)
    generate.add_argument("--public-header", type=Path, required=True)
    sign = commands.add_parser("issue", help="Issue one machine's 14-day trial")
    sign.add_argument("--key", type=Path, required=True)
    identity = sign.add_mutually_exclusive_group(required=True)
    identity.add_argument("--request", type=Path)
    identity.add_argument("--machine-id")
    sign.add_argument("--start", help="ISO 8601 date with timezone; defaults to now (UTC)")
    sign.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.command == "keygen":
            keygen(args.key, args.public_header)
            print("Created private issuing key and public verification header. Keep the private key private.")
            return
        machine = args.machine_id
        if args.request:
            request = json.loads(args.request.read_text(encoding="utf-8"))
            if request.get("product") != "GrainsDosage-Trial-v1":
                raise ValueError("Unrecognized machine request")
            machine = request["machine_id"]
        start = dt.datetime.fromisoformat(args.start.replace("Z", "+00:00")) if args.start else dt.datetime.now(dt.timezone.utc)
        contents = issue(args.key, machine, start)
        with args.out.open("xb") as output:
            output.write(contents)
        print(f"License saved to {args.out}; expires {(start + dt.timedelta(days=14)).isoformat()}")
    except (ValueError, OSError, KeyError, TypeError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()
