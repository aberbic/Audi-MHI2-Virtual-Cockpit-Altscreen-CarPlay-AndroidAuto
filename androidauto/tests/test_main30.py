"""Synthetic configuration tests; no OEM config or live unit required."""
from pathlib import Path
import subprocess
import sys
import tempfile

script = Path(__file__).resolve().parents[1] / 'scripts/prepare-main30.py'
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'source.json'
    target = Path(directory) / 'target.json'
    def run():
        return subprocess.run([sys.executable, script, source, target], capture_output=True)
    before = b'// retain comments\n{"supportedFrameRates": [30, 60], "other":60}\n'
    source.write_bytes(before)
    assert run().returncode == 0
    expected = before.replace(b'[30, 60]', b'[30]')
    assert target.read_bytes() == expected
    assert source.read_bytes() == before
    assert run().returncode != 0  # never overwrite an existing output
    assert target.read_bytes() == expected
    target.unlink()
    for invalid in (b'{}', expected, before + before):
        source.write_bytes(invalid)
        assert run().returncode != 0
        assert not target.exists()
print('PASS: main 30fps transformation preserves other bytes and rejects ambiguous/existing outputs')
