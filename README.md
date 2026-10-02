# Audi MHI2 Virtual Cockpit AltScreen — CarPlay & Android Auto

Open development of CarPlay and Android Auto secondary navigation video on the Audi Virtual Cockpit, using the **Nvidia Tegra MHI2** head unit.

**CarPlay works. Android Auto works at 720p.** This is a community development snapshot from a single vehicle, not a certified or universally compatible retrofit. Read the limitations before installing.

## Compatibility

Tested on **MHI2_ER_AUG22_K3344 / MU1438**, Audi MHI2 High, Nvidia Tegra, QNX 6.5, with an LVDS-connected Virtual Cockpit. Classic gauge layout was the accepted configuration.

**Tegra only. Not MHI2Q/Qualcomm, MHI3, or a generic MIB2 package.** MU1326 / `MHI2_ER_AU37x_P5089` was not tested in this project. Even another Tegra train requires its own validation and firmware adaptation. Installation is locked to exact MU1438 file hashes; do not remove those checks to try another train.

## Status

| Component | Current status |
| --- | --- |
| CarPlay main screen + cluster map | Working; automatic cluster takeover while the secondary stream is available |
| CarPlay smoothness | Working after complete-frame handoff, decoder startup ordering and TCP_NODELAY fixes |
| Apple Maps steering-wheel zoom | Working in the tested setup |
| Waze steering-wheel zoom | App ignored the zoom command in testing |
| Android Auto main screen + cluster map | Working at **1280×720**, scaled to fill the classic map area |
| AA centre-console controls | Owner confirmed working at 720p; failed in the prior 1080p configuration |
| AA steering-wheel zoom | Not working; experimental forwarding disabled by default |
| Native Audi map/label suppression | Working with automatic context restoration and backend-specific title |
| Touchpad → CarPlay directions/select | Port and diagnostics included; **not working yet on the test vehicle**; optional, not in default install |
| CarPlay native maneuver arrows / ETA | **Experimental, not integrated into the display** |
| Metadata reception | Real Apple Maps maneuvers captured, but adapter caused disconnect/reconnect failures and was rolled back |
| Other layouts / long-term reliability | Not comprehensively validated |

The default build uses the working **720p AA baseline**, not the older 1080p snapshot. The 720p image is visibly softer; the 1080p experiment affected controls and earlier builds exhausted decoder buffers. Resolution/load is implicated, but the exact cause of the control failure is not proven.

Steering-wheel centre-button actions, AA zoom, altitude suppression, other gauge layouts, dependable metadata lifecycle and native instruction rendering remain pending. CarPlay's richer-ETA presentation request was tried without a visible improvement; it is not enabled by this installer.

## Install

See the [installation and rollback guide](docs/INSTALL.md) for **SSH** and **M.I.B. toolbox/SD-card** workflows.

This is **not a download-and-flash image**. Native project binaries are included, but proprietary firmware executables and reconstructed Audi/Harman HMI classes are not. Build the HMI add-on and prepare a private, hash-checked installation bundle from your own exact firmware first.

The consolidated public installer is newly packaged and has **not been validated as a fresh end-to-end installation on another car**. The underlying runtimes were installed and tested incrementally. Treat installation as an expert-only, reversible development procedure.

Park safely, use appropriate power support, keep known-good backups and a working recovery method, and never experiment while driving. Scripts do not reboot the MMI, stop processes, alter credentials, modify FEC/VIN/license data, or enable metadata experiments. A manual MMI restart is needed after installing HMI changes.

## Repository

| Directory | Contents |
| --- | --- |
| [`carplay/`](carplay) | Hook, NvSS renderer, supervisor, firmware-loader delta and native tests |
| [`androidauto/`](androidauto) | AA endpoints, frame transport, 720p renderer, supervisor and tests |
| [`hmi/`](hmi) | Shared cluster/context/label bridge and a patcher for locally supplied MU1438 classes |
| [`touchpad/`](touchpad) | MU1438 touchpad port and bounded event diagnostics; unfinished |
| [`experimental/metadata/`](experimental/metadata) | Tegra iAP2 negotiation/observer sources, protocol tests and failure notes |
| [`experimental/factory-navigation-observer/`](experimental/factory-navigation-observer) | Factory navigation call recorder and bytecode instrumentation |
| [`toolbox/`](toolbox) | M.I.B. custom-script entry points for locally prepared bundles |
| [`profiles/`](profiles) | Exact tested firmware fingerprints and geometry |

Read [architecture](docs/ARCHITECTURE.md), [development/handoff](docs/DEVELOPMENT.md) and [provenance](NOTICE).

## Build and test

Prerequisites: Python 3, a C compiler for host tests, a current JDK for bytecode tooling, Docker with Java 8, and a lawfully obtained QNX 6.5 ARMv7 toolchain. See [build instructions](docs/BUILD.md).

```sh
python3 scripts/verify-public.py
bash scripts/test.sh
bash scripts/build-hmi.sh       # requires your private inputs/
bash scripts/build-tools.sh
python3 scripts/prepare-bundle.py
```

No live vehicle is needed for host tests or packaging. Keep generated `inputs/`, `out/`, firmware backups, logs and captures private.

## Credits and license

The CarPlay hook builds on [harman-f/mhi2_altscreen_carplay](https://github.com/harman-f/mhi2_altscreen_carplay). Protocol and architecture references include [luka-dev/mib2q-carplay-rgi](https://github.com/luka-dev/mib2q-carplay-rgi), [f-io/LIVI](https://github.com/f-io/LIVI), and the touchpad concept from [binarybase/mhi2-touchpad-fix](https://github.com/binarybase/mhi2-touchpad-fix). Thanks to those projects for making the research accessible.

Project code is distributed under [GPL-3.0](LICENSE), preserving upstream obligations; see [NOTICE](NOTICE). No rights to Audi/Harman firmware, closed vendor packages or proprietary SDKs are granted. No such firmware dumps, vendor binaries, generated HMI JARs, credentials or private navigation captures are included.
