#!/usr/bin/env python3
"""Check the actual wrapper/engine load commands, not only CMake settings."""
import pathlib
import plistlib
import struct
import sys


def version(value):
    return (value >> 16, (value >> 8) & 255, value & 255)


def check_bundle(bundle):
    with (bundle / 'Contents/Info.plist').open('rb') as stream:
        info = plistlib.load(stream)
    minimum = tuple(map(int, info['LSMinimumSystemVersion'].split('.')))
    assert minimum[:2] <= (10, 14), (bundle, 'plist minimum', minimum)
    data = (bundle / 'Contents/MacOS' / info['CFBundleExecutable']).read_bytes()
    magic, count = struct.unpack_from('>II', data)
    assert magic == 0xCAFEBABE and count == 2, 'Expected universal Intel/ARM binary'
    found = set()
    for index in range(count):
        cpu, _, offset, size, _ = struct.unpack_from('>IIIII', data, 8 + index * 20)
        binary = data[offset:offset + size]
        assert struct.unpack_from('<I', binary)[0] == 0xFEEDFACF
        command_count = struct.unpack_from('<I', binary, 16)[0]
        position = 32
        minima = []
        for _ in range(command_count):
            command, length = struct.unpack_from('<II', binary, position)
            assert length >= 8 and position + length <= len(binary)
            if command == 0x32:  # LC_BUILD_VERSION
                platform, target = struct.unpack_from('<II', binary, position + 8)
                assert platform == 1, 'Expected macOS'
                minima.append(version(target))
            elif command == 0x24:  # LC_VERSION_MIN_MACOSX
                minima.append(version(struct.unpack_from('<I', binary, position + 8)[0]))
            position += length
        limit = {0x01000007: (10, 14, 0), 0x0100000C: (11, 0, 0)}[cpu]
        assert len(minima) == 1 and minima[0] <= limit, (bundle, hex(cpu), minima, limit)
        found.add(cpu)
        print(f'{bundle.name}: {hex(cpu)} minimum {minima[0]} OK')
    assert found == {0x01000007, 0x0100000C}


if __name__ == '__main__':
    root = pathlib.Path(sys.argv[1])
    check_bundle(root)
    check_bundle(root / 'Contents/Resources/plugin.vst3')
