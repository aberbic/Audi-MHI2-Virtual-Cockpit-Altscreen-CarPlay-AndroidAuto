#!/usr/bin/env python3
"""Fetch verified baseline installation components; no SDK or Java required."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('release_download', ROOT / 'scripts/fetch-firmware.py')
download = importlib.util.module_from_spec(spec)
spec.loader.exec_module(download)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, help='verify/install a manually downloaded component archive')
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'prebuilt/mu1438-components.json').read_text())
    destination = ROOT / 'out/prebuilt'
    if args.archive:
        download.install_archive(args.archive, destination, manifest)
        return
    (ROOT / 'out').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='components-download.', dir=ROOT / 'out') as directory:
        subprocess.run(['gh', 'release', 'download', manifest['release'],
                        '--repo', manifest['repository'], '--pattern', manifest['archive'],
                        '--dir', directory], check=True)
        download.install_archive(Path(directory) / manifest['archive'], destination, manifest)


if __name__ == '__main__':
    main()
