#!/usr/bin/env python3
"""Check staging bundles before packaging; never print scanned file contents."""
import base64, pathlib, re, sys
root=pathlib.Path(__file__).resolve().parents[1]
header=(root/'src/license_public_key.h').read_text()
body=header.split('{',2)[-1].split('}',1)[0]
# Public header is a namespace followed by the byte array initializer.
public=bytes(int(x,0) for x in re.findall(r'0x[0-9a-fA-F]+|\b\d+\b',body))
windows=base64.b64decode((root/'config/windows-trial-public.b64').read_text())
seen=False
for argument in sys.argv[1:]:
    path=pathlib.Path(argument)
    assert path.exists(), ('Missing staging path',str(path))
    files=[path] if path.is_file() else [p for p in path.rglob('*') if p.is_file()]
    for file in files:
        assert file.suffix.lower() not in {'.pem','.p12','.pfx','.key','.sqlite','.db'}, ('Forbidden signing/ledger file',str(file))
        data=file.read_bytes()
        assert not re.search(rb'-----BEGIN (?:RSA |ENCRYPTED |EC |OPENSSH )?PRIVATE KEY-----',data), ('Private key marker',str(file))
        if public in data or windows in data:seen=True
assert seen,'Expected production public key not found in the staged binaries'
print('PASS: production public key present; no private-key files, PEM private keys or publisher ledger in staging')
