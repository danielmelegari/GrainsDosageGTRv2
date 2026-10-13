"""Verify Intel / Mojave deployment and an explicit public-only bundle allowlist."""
import pathlib, plistlib, subprocess, sys
app = pathlib.Path(sys.argv[1])
expected = {'Contents/MacOS/GrainsDosage License Generator', 'Contents/Info.plist', 'Contents/Resources/Guida.md', 'Contents/_CodeSignature/CodeResources'}
files = {str(p.relative_to(app)) for p in app.rglob('*') if p.is_file()}
assert files == expected, ('Unexpected bundle contents', files ^ expected)
for p in app.rglob('*'):
    assert not p.is_symlink(), p
    if p.is_file():
        data = p.read_bytes()
        for marker in (b'-----BEGIN PRIVATE KEY-----', b'-----BEGIN RSA PRIVATE KEY-----', b'-----BEGIN ENCRYPTED PRIVATE KEY-----'):
            assert marker not in data, ('Private key in bundle', p)
exe = str(app / 'Contents/MacOS/GrainsDosage License Generator')
assert subprocess.check_output(['lipo', '-archs', exe], text=True).strip() == 'x86_64'
load = subprocess.check_output(['otool', '-l', exe], text=True)
assert ('version 10.14' in load or 'minos 10.14' in load), load
info = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
assert info['LSMinimumSystemVersion'] == '10.14'
print('PASS: x86_64, macOS 10.14 minimum, public-only bundle allowlist')
