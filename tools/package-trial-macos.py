#!/usr/bin/env python3
"""Wrap validated commercial archives in a system-wide, non-relocatable installer."""
import argparse
from pathlib import Path
import plistlib
import subprocess
import tempfile
import xml.etree.ElementTree as ET


def run(*args):
    subprocess.run([str(x) for x in args], check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--edition', choices=['trial', 'licensed'], default='trial')
    args = parser.parse_args()
    edition = args.edition
    label = 'Trial' if edition == 'trial' else 'Licensed'
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='grains-trial-pkg-') as tmp:
        work = Path(tmp)
        root = work / 'root'
        bundles = []
        for suffix, folder in [('vst3', 'VST3'), ('component', 'Components')]:
            destination = root / 'Library/Audio/Plug-Ins' / folder
            destination.mkdir(parents=True)
            run('ditto', '-x', '-k', args.source / f'GrainsDosage-{edition}.{suffix}.zip', destination)
            bundle = destination / f'GrainsDosage.{suffix}'
            run('codesign', '--verify', '--deep', '--strict', bundle)
            run('python3', Path(__file__).with_name('check-au-deployment.py'), bundle)
            bundles.append(bundle)
        version = None
        for bundle in bundles:
            info = plistlib.loads((bundle / 'Contents/Info.plist').read_bytes())
            assert info['CFBundlePackageType'] == 'BNDL', 'Refuse application-classified plugins'
            current = info['CFBundleShortVersionString']
            assert version is None or current == version, 'AU/VST3 version mismatch'
            version = current
            if edition == 'trial':
                assert (bundle / 'Contents/Resources' / ('plugin.vst3/Contents/Resources/' if bundle.suffix == '.component' else '') / 'GrainsDosageTrialHelper').is_file()
        run('python3', Path(__file__).with_name('check-public-only.py'),
            *(['--automatic-trial'] if edition == 'trial' else []), *bundles)
        # pkgbuild must not relocate an update to a user's backup/old bundle.
        components = work / 'components.plist'
        run('pkgbuild', '--analyze', '--root', root, components)
        entries = plistlib.loads(components.read_bytes())
        for entry in entries:
            entry['BundleIsRelocatable'] = False
            entry['BundleHasStrictIdentifier'] = True
            entry['BundleOverwriteAction'] = 'upgrade'
        components.write_bytes(plistlib.dumps(entries))
        payload = work / 'payload.pkg'
        identifier = 'com.danielmelegari.grainsdosage.' + edition + '.installer'
        run('pkgbuild', '--root', root, '--component-plist', components,
            '--identifier', identifier, '--version', version, '--install-location', '/', payload)
        resources = Path(__file__).resolve().parents[1] / 'installer/trial/resources'
        if edition == 'licensed':
            resources = work / 'licensed-resources'
            resources.mkdir()
            (resources / 'welcome.html').write_text(
                '<html><body><h1>GrainsDosage Licensed</h1>'
                '<p>Installs Audio Unit and VST3. Close your DAW before installing.</p>'
                '<p>Intel: macOS Mojave 10.14 or later. Apple Silicon: macOS 11 or later.</p>'
                '<p>Replaces the Trial edition. Open ACTIVATE in the plugin to enter your device activation.</p>'
                '<p>Presets and saved sessions are preserved.</p></body></html>', encoding='utf-8')
        distribution = ET.Element('installer-gui-script', {'minSpecVersion': '2'})
        if (resources / 'background.png').is_file():
            ET.SubElement(distribution, 'background', {'file': 'background.png', 'alignment': 'left', 'scaling': 'proportional'})
        if (resources / 'background-dark.png').is_file():
            ET.SubElement(distribution, 'background-darkAqua', {'file': 'background-dark.png', 'alignment': 'left', 'scaling': 'proportional'})
        ET.SubElement(distribution, 'title').text = 'GrainsDosage 14-Day Trial' if edition == 'trial' else 'GrainsDosage Licensed'
        if (resources / 'welcome.html').is_file():
            ET.SubElement(distribution, 'welcome', {'file': 'welcome.html', 'mime-type': 'text/html'})
        ET.SubElement(distribution, 'options', {'customize': 'never', 'require-scripts': 'false', 'hostArchitectures': 'x86_64,arm64'})
        ET.SubElement(distribution, 'domains', {'enable_anywhere': 'false', 'enable_currentUserHome': 'false', 'enable_localSystem': 'true'})
        volumes = ET.SubElement(distribution, 'volume-check')
        ET.SubElement(ET.SubElement(volumes, 'allowed-os-versions'), 'os-version', {'min': '10.14'})
        ET.SubElement(ET.SubElement(distribution, 'choices-outline'), 'line', {'choice': edition})
        choice = ET.SubElement(distribution, 'choice', {'id': edition, 'visible': 'false', 'title': 'VST3 and Audio Unit'})
        ET.SubElement(choice, 'pkg-ref', {'id': identifier})
        ET.SubElement(distribution, 'pkg-ref', {'id': identifier, 'version': version, 'onConclusion': 'none'}).text = 'payload.pkg'
        xml = work / 'Distribution.xml'
        ET.ElementTree(distribution).write(xml, encoding='utf-8', xml_declaration=True)
        resource_args = ['--resources', resources] if resources.is_dir() else []
        run('productbuild', '--distribution', xml, '--package-path', work, *resource_args,
            args.output / f'GrainsDosage-{label}-{version}-macOS-Universal.pkg')


if __name__ == '__main__':
    main()
