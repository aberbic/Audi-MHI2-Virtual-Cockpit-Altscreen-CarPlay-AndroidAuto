# Installation and rollback

## Read this first

Only **Nvidia Tegra MHI2_ER_AUG22_K3344 / MU1438** with the exact hashes in `profiles/mu1438.json` is supported by these scripts. MU1326 and Qualcomm are not supported. A similar model name is not sufficient.

The working runtimes were tested on one vehicle. These consolidated installation entry points are **not yet fresh-car validated**. Touchpad diagnostics and routing metadata are excluded from the default install. Do not substitute experimental binaries into a baseline bundle.

You need existing, authorized access to your head unit, a backup/recovery method, adequate power support and a parked vehicle. This guide does not install an SSH server, modify activation/FEC data, or replace firmware trains. Take a full backup using your established M.I.B. workflow before proceeding.

## 1. Prepare private inputs

Create ignored `inputs/` locally and obtain these from your own unit:

| Local file | Unit source |
| --- | --- |
| `inputs/dio_manager.stock` | Original `/mnt/app/eso/bin/apps/dio_manager` |
| `inputs/gal.stock` | Original `/mnt/app/eso/bin/apps/gal` |
| `inputs/lsd.jxe` | Original `/ifs/lsd.jxe` |
| `inputs/mu1438-stock.jar` | Convert that exact JXE with jxe2jar |

Use genuine original executables, not an already-patched live copy. If modifications are already installed, recover the original files from your own backups. Hash mismatches stop bundle generation; do not edit hashes to make an unsupported firmware pass.

With an existing SSH alias, a read-only copy looks like:

```sh
mkdir -p inputs
ssh mib 'cat /ifs/lsd.jxe' > inputs/lsd.jxe
```

Configure SSH outside this repository. Ancient firmware may require host-specific `HostKeyAlgorithms +ssh-rsa` and `PubkeyAcceptedAlgorithms +ssh-rsa`. Verify the host key and keep credentials out of Git; do not disable host-key checking globally.

Follow [BUILD.md](BUILD.md) to supply ASM/OSGi dependencies and build the local HMI add-on. Then:

```sh
bash scripts/build-hmi.sh
bash scripts/build-tools.sh
python3 scripts/prepare-bundle.py
```

This creates `out/bundle/` and `out/mib-sd/mod/`. They contain locally reconstructed proprietary files: **do not upload either directory to GitHub or share them as a release**. Move an old bundle aside before preparing another one; the generator refuses to overwrite it.

## 2A. Install over SSH

Park, disconnect both phones, and let CarPlay/AA/USB children exit. If an iAP2 process remains stuck, restore your known-good driver and restart MMI manually before continuing. Do not kill shared services blindly.

```sh
bash scripts/install-ssh.sh --check mib
bash scripts/install-ssh.sh --install mib
```

The first command stages files in flat QNX RAM and checks compatibility without changing flash. The second performs the guarded installation. It:

1. Checks original HMI, AirPlay, AA receiver and iAP2-driver fingerprints.
2. Allows only known stock or project-patched CarPlay/AA loader hashes.
3. Verifies all nine payload files and fixed destination paths.
4. Saves prior files under `/mnt/app/root/altscreen-backups/<timestamp>-<pid>/`.
5. Stages all replacements, verifies them, then publishes by rename.
6. Returns `/mnt/app` to read-only and prints the recovery directory.

Save that directory and copy it to your own computer. No script restarts MMI automatically. **Restart MMI manually while parked**, then reconnect one phone.

The stock MU1438 `lsd.sh` loads JARs from its `jars/` directory before `lsd.jxe`, so no boot script edit is needed on that exact layout. Inspect other installed HMI patches for class conflicts; this package neither removes nor guarantees compatibility with unrelated modifications.

## 2B. Install using M.I.B. / SD card

Use an already working [M.I.B. More Incredible Bash](https://github.com/Mr-MIBonk/M.I.B._More-Incredible-Bash) SD setup. Follow that project's power, backup and recovery guidance. This repository supplies a custom-script module, not a firmware-update image or a generic “MIB2 Toolbox” package.

1. Back up existing SD `mod/custom.sh` and `mod/command.sh`; another custom script may use them.
2. Copy the contents of your locally generated `out/mib-sd/` onto the M.I.B. SD card. The expected layout is `mod/custom.sh`, `mod/command.sh`, and `mod/altscreen/altscreen-*`.
3. Park and disconnect projection phones before running it.
4. In M.I.B. Advanced Settings, run the custom/individual script. Menu wording varies: older releases invoke `command.sh`, newer ones invoke `custom.sh`. The wrapper supports both entry points and forwards RCC launches to MMX when needed.
5. Read the compatibility result and retain the printed on-unit backup directory. The same guarded installer used for SSH performs the writes.
6. Restart MMI manually, then test one phone at a time.

For a check-only invocation from a shell, run `/bin/sh <SD-mount>/mod/custom.sh --check`. Do not put this payload into the red SWDL firmware-update menu.

## 3. Acceptance checks

While parked, verify:

- Centre projection connects, audio works, and **rotary, select and back controls all work**.
- Cluster map appears automatically in classic layout.
- Stop/start guidance and disconnect/reconnect do not cause context oscillation.
- Apple Maps cluster zoom works where supported; AA zoom is not expected to work.
- Native cluster map returns after projection ends; gauges and warning/navigation-independent contexts remain available.
- Check startup again after a manual MMI restart. Record firmware hashes and phone/app versions for reports.

Do not start road testing if normal controls, warnings or display recovery are unreliable.

## Rollback

Disconnect phones first. From the exact printed backup directory:

```sh
ssh mib '/bin/sh /mnt/app/root/altscreen-backups/REPLACE-WITH-YOUR-BACKUP/altscreen-install.sh --restore --parked'
```

This restores the prior files, which may themselves have been modified; it is not automatically a factory reset. Files that did not exist before installation are moved into the backup directory, not destroyed. Unexpected later modifications cause the rollback to stop rather than overwrite them. Restart MMI manually afterward.

For an interrupted installation, retain the SD payload and backup directory. Do not repeat writes, delete backups, or bypass a hash failure. Restore through the saved script/recovery method and inspect the error first.

## Optional touchpad work

The touchpad port loaded and bound to CarPlay but did not translate input on the test car. Do **not** include it expecting a finished fix. Developers can build it with `bash touchpad/build.sh` and explicitly prepare a bundle with `python3 scripts/prepare-bundle.py --touchpad`. Its event recorder needs a separate opt-in RAM marker after restart; see [touchpad/README.md](../touchpad/README.md).

There is deliberately no standard installer option for experimental routing metadata. Its disconnect/reconnect failure remains unresolved.
