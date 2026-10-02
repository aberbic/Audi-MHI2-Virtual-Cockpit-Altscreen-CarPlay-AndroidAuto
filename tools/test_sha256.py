from pathlib import Path
import hashlib, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
tool = root / 'out/tools/sha256-host'
with tempfile.TemporaryDirectory(prefix='altscreen-sha-test-') as folder:
    file = Path(folder) / 'fixture'
    for size in [0, 1, 55, 56, 63, 64, 65, 127, 128, 4096, 65537]:
        data = bytes(i % 251 for i in range(size))
        file.write_bytes(data)
        digest = hashlib.sha256(data).hexdigest()
        assert subprocess.check_output([tool, file], text=True).strip() == digest
        subprocess.run([tool, digest, file], check=True)
        assert subprocess.run([tool, '0' * 64, file], capture_output=True).returncode == 1
print('PASS: SHA256 boundary vectors, large input and mismatch detection')
