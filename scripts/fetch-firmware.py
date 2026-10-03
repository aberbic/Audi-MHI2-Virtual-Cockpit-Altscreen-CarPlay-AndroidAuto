#!/usr/bin/env python3
"""Download the published MU1438 build inputs; verify before writing inputs/."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def digest(stream):
    h = hashlib.sha256()
    for block in iter(lambda: stream.read(1024 * 1024), b''):
        h.update(block)
    return h.hexdigest()


def install_archive(archive, destination, manifest):
    if archive.stat().st_size != manifest['archive_bytes']:
        raise ValueError('Archive size mismatch')
    with archive.open('rb') as stream:
        if digest(stream) != manifest['archive_sha256']:
            raise ValueError('Archive checksum mismatch')
    expected = manifest['files']
    with tarfile.open(archive, 'r:gz') as bundle:
        members = bundle.getmembers()
        if len(members) != len(expected) or {m.name for m in members} != set(expected):
            raise ValueError('Unexpected archive contents')
        # Validate every member and existing destination before copying any file.
        for member in members:
            spec = expected[member.name]
            if (not member.isfile() or Path(member.name).name != member.name or
                    member.name in ('.', '..') or member.size != spec['bytes']):
                raise ValueError('Invalid firmware member: ' + member.name)
            with bundle.extractfile(member) as stream:
                if digest(stream) != spec['sha256']:
                    raise ValueError('Firmware checksum mismatch: ' + member.name)
            target = destination / member.name
            if target.is_symlink():
                raise ValueError('Refusing a symlink destination: ' + member.name)
            if target.exists():
                if not target.is_file():
                    raise ValueError('Destination is not a file: ' + member.name)
                with target.open('rb') as stream:
                    if digest(stream) != spec['sha256']:
                        raise ValueError('Existing input differs; move it aside first: ' + member.name)
        destination.mkdir(parents=True, exist_ok=True)
        for member in members:
            target = destination / member.name
            if not target.exists():
                with bundle.extractfile(member) as source, target.open('xb') as output:
                    shutil.copyfileobj(source, output)
                with target.open('rb') as stream:
                    if digest(stream) != expected[member.name]['sha256']:
                        raise ValueError('Written input checksum mismatch: ' + member.name)
    print('Verified release files:', destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, help='verify/install an already downloaded archive')
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'firmware/mu1438-inputs.json').read_text())
    if args.archive:
        install_archive(args.archive, ROOT / 'inputs', manifest)
        return
    (ROOT / 'out').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='firmware-download.', dir=ROOT / 'out') as directory:
        subprocess.run(['gh', 'release', 'download', manifest['release'],
                        '--repo', manifest['repository'], '--pattern', manifest['archive'],
                        '--dir', directory], check=True)
        install_archive(Path(directory) / manifest['archive'], ROOT / 'inputs', manifest)


if __name__ == '__main__':
    main()
