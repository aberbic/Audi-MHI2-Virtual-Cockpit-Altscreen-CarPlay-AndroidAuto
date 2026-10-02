"""Synthetic archive tests; no OEM inputs or network required."""
import hashlib
import importlib.util
import io
from pathlib import Path
import tarfile
import tempfile

spec = importlib.util.spec_from_file_location('fetch_firmware', Path(__file__).with_name('fetch-firmware.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    archive = root / 'example.tar.gz'
    contents = {'a.stock': b'synthetic a', 'b.stock': b'synthetic b'}
    manifest = {'files': {name: {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                           for name, data in contents.items()}}
    with tarfile.open(archive, 'w:gz') as bundle:
        for name, data in contents.items():
            item = tarfile.TarInfo(name); item.size = len(data)
            bundle.addfile(item, io.BytesIO(data))
    manifest['archive_bytes'] = archive.stat().st_size
    manifest['archive_sha256'] = hashlib.sha256(archive.read_bytes()).hexdigest()
    destination = root / 'inputs'
    module.install_archive(archive, destination, manifest)
    module.install_archive(archive, destination, manifest)  # identical inputs preserved
    assert all((destination / name).read_bytes() == data for name, data in contents.items())
    (destination / 'a.stock').unlink()
    (destination / 'b.stock').write_bytes(b'keep this different input')
    try:
        module.install_archive(archive, destination, manifest)
        raise AssertionError('expected conflicting-input rejection')
    except ValueError:
        pass
    assert not (destination / 'a.stock').exists()  # no partial writes before validation
    assert (destination / 'b.stock').read_bytes() == b'keep this different input'
    manifest['archive_sha256'] = '0' * 64
    try:
        module.install_archive(archive, root / 'rejected', manifest)
        raise AssertionError('expected checksum rejection')
    except ValueError:
        pass
    assert not (root / 'rejected').exists()
print('PASS: firmware download verification, idempotence and non-overwriting input preflight')
