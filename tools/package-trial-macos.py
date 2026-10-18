#!/usr/bin/env python3
"""Wrap validated Trial archives in a system-wide, non-relocatable installer."""
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
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='grains-trial-pkg-') as tmp:
        work = Path(tmp)
        root = work / 'root'
        bundles = []
        for suffix, folder in [('vst3', 'VST3'), ('component', 'Components')]:
            destination = root / 'Library/Audio/Plug-Ins' / folder
            destination.mkdir(parents=True)
            run('ditto', '-x', '-k', args.source / f'GrainsDosage-trial.{suffix}.zip', destination)
            bundle = destination / f'GrainsDosage.{suffix}'
            run('codesign', '--verify', '--deep', '--strict', bundle)
            bundles.append(bundle)
        version = None
        for bundle in bundles:
            info = plistlib.loads((bundle / 'Contents/Info.plist').read_bytes())
            assert info['CFBundlePackageType'] == 'BNDL', 'Refuse application-classified plugins'
            current = info['CFBundleShortVersionString']
            assert version is None or current == version, 'AU/VST3 version mismatch'
            version = current
            assert (bundle / 'Contents/Resources' / ('plugin.vst3/Contents/Resources/' if bundle.suffix == '.component' else '') / 'GrainsDosageTrialHelper').is_file()
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
        identifier = 'com.danielmelegari.grainsdosage.trial.installer'
        run('pkgbuild', '--root', root, '--component-plist', components,
            '--identifier', identifier, '--version', version, '--install-location', '/', payload)
        resources = Path(__file__).resolve().parents[1] / 'installer/trial/resources'
        distribution = ET.Element('installer-gui-script', {'minSpecVersion': '2'})
        if (resources / 'background.png').is_file():
            ET.SubElement(distribution, 'background', {'file': 'background.png', 'alignment': 'left', 'scaling': 'proportional'})
        if (resources / 'background-dark.png').is_file():
            ET.SubElement(distribution, 'background-darkAqua', {'file': 'background-dark.png', 'alignment': 'left', 'scaling': 'proportional'})
        ET.SubElement(distribution, 'title').text = 'GrainsDosage 14-Day Trial'
        if (resources / 'welcome.html').is_file():
            ET.SubElement(distribution, 'welcome', {'file': 'welcome.html', 'mime-type': 'text/html'})
        ET.SubElement(distribution, 'options', {'customize': 'never', 'require-scripts': 'false', 'hostArchitectures': 'x86_64,arm64'})
        ET.SubElement(distribution, 'domains', {'enable_anywhere': 'false', 'enable_currentUserHome': 'false', 'enable_localSystem': 'true'})
        volumes = ET.SubElement(distribution, 'volume-check')
        ET.SubElement(ET.SubElement(volumes, 'allowed-os-versions'), 'os-version', {'min': '10.14'})
        ET.SubElement(ET.SubElement(distribution, 'choices-outline'), 'line', {'choice': 'trial'})
        choice = ET.SubElement(distribution, 'choice', {'id': 'trial', 'visible': 'false', 'title': 'VST3 and Audio Unit'})
        ET.SubElement(choice, 'pkg-ref', {'id': identifier})
        ET.SubElement(distribution, 'pkg-ref', {'id': identifier, 'version': version, 'onConclusion': 'none'}).text = 'payload.pkg'
        xml = work / 'Distribution.xml'
        ET.ElementTree(distribution).write(xml, encoding='utf-8', xml_declaration=True)
        resource_args = ['--resources', resources] if resources.is_dir() else []
        run('productbuild', '--distribution', xml, '--package-path', work, *resource_args,
            args.output / f'GrainsDosage-Trial-{version}-macOS-Universal.pkg')


if __name__ == '__main__':
    main()
