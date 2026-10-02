# Android Auto image-quality plan

Saved and updated 2026-10-02. Status: **owner accepted the fuller-1080p / main-screen-30fps profile and reported the resolution problem solved.** The exact tested AA binaries are now the repository baseline; private bundle generation includes the paired main-screen configuration. Packed 720p provided only a slight improvement and was superseded.

## Accepted vehicle result

- Captured secondary H.264 confirms **1920×1080**, Constrained Baseline, level 4.0.
- Renderer readback confirms crop (0,180,1920,720) to output (0,0,1440,540).
- The recorded run ended cleanly after **1,322 frames**. Later 120-frame arrival windows correspond to roughly 27–29 fps; this is not a display-presentation measurement.
- No decoder failures/retries appeared in that run; sampled transport queues were zero. Decoder-feed calls peaked at 2 ms, not a measurement of total decode/display latency.
- Stock main service discovery advertised 720p/480p options at the 30fps enum after the config change; actual main-stream selection/cadence was not captured.
- Installed hook, renderer and configuration were read back and byte-compared to the tested local artifacts before promotion. HMI was not changed during this experiment.
- Owner feedback: “this WORKS! the resolution problem has been solved.” That confirms visible quality, not exhaustive control, reconnect or long-duration acceptance.
- Private video/event captures remain outside Git. Temporary video and touchpad trace enable markers were removed after collection.

Exact promoted SHA-256 values:

| Artifact | SHA-256 |
| --- | --- |
| `libaa_observe.so` | `d4cbdc201940fda58e17d7e51c59036dba1440a97625b224ffccff1dec54e961` |
| `aa_renderer` | `f1a9c72e6f7451c3c07ff876da5187f06579a72559e0be369d9918f879b3452c` |
| Private generated `gal.json` | `1bddd7e893850c1087eb36bd19ba9a5b270e8fb879720513cf5ad8b3e860ab9a` |

The previous 720p baseline is retained in Git history and vehicle backups. Public installer packaging remains unvalidated on a fresh car; geometry and the main frame-rate limit changed together, so neither alone is proven to explain the previous control failure.

## Second experiment — now the accepted baseline

The owner requested proceeding after the modest packed-720p result. `AA_PACKED_1080=1` configures a 1920×1080 stream with crop (0,180,1920,720), downsampled to 1440×540. DPI 192 and insets (102,194,680,680) retain the original logical UI size and physical edges. Input-service bytes, codec, cluster 30fps declaration, decoder depth and theme are unchanged in tests.

The main-screen `gal.json` frame-rate list was changed from `[30, 60]` to `[30]` as an explicit resource-budget measure. The previous negotiated main rate is unknown; this change must not be described as a measured reduction from 60fps. All other config bytes were preserved. The AA binary pair and the original config are backed up locally and on-unit; HMI is unchanged.

Diagnostics log stock advertised video configurations plus bounded cluster arrival-window bytes/time and GAL CPU time. An existing HMI trace can show console events, but does not by itself prove delivery to the phone. Video capture remains bounded at 8 MiB. No new main-router or input interception was added.

Build: `bash androidauto/scripts/build-packed-1080.sh`.
For normal installation use [INSTALL.md](INSTALL.md). The retained development script `deploy-packed-1080.sh` describes the historical transition from packed 720p and has state-specific hash guards; it is not a general release installer. Its restore returns the packed-720 pair and the original main-frame-rate configuration.

## First vehicle result

- Captured H.264 confirms Constrained Baseline, level 3.1, coded/displayed dimensions 1280×720; 740 frames parsed.
- Renderer readback confirms crop (0,120,1280,480) to (0,0,1440,540).
- No decoder failures/retries appeared in the captured session; sampled queue reports were zero. Decoder-feed calls peaked at 6 ms, which does not measure complete hardware decode/display latency.
- Disconnect closed the decoder successfully after 740 frames.
- Source-frame inspection shows fine-label/road-edge aliasing already present in the phone-rendered picture. No simultaneous cluster screenshot was captured, so additional scaler blur is not quantified.
- The elementary H.264 capture has no trustworthy wall-clock presentation timestamps. FFprobe's guessed rates must not be treated as actual cadence measurements.
- Owner feedback: slight improvement, still too blurry. Main-screen button acceptance was not explicitly reconfirmed in that feedback.
- The private capture is retained locally outside Git. The temporary capture-enable marker was removed afterward; this profile was later replaced by the accepted 1080p pair.

The subsequent accepted candidate uses a fuller 1920×720 map area inside a 1920×1080 stream and downsamples to 1440×540, rather than repeating the old 1440×540 crop inside 1080p. It provides 2.25× the useful samples of packed 720p, but also 2.25× the coded pixels per frame. Extended controls/recovery checks remain open.

## Historical first experiment

`AA_PACKED_720=1` selects shared geometry for the advertisement and renderer. Coded size stays 1280×720 at 30 fps; source crop is (0,120,1280,480). DPI is 128, real density 113, and content/stable insets are (top=68,bottom=129,left=453,right=453), scaled from the accepted baseline. Input-service bytes, zoom reports, codec, decoder depth, theme and display destination are unchanged in tests.

Build/test: `bash androidauto/scripts/build-packed-720.sh`.
Install/restore: `bash androidauto/scripts/deploy-packed-720.sh --install|--restore` with AA disconnected. Both binaries are backed up and hash-checked as a pair; HMI is checked unchanged. The experiment arms an existing opt-in capture capped at 8 MiB. Disable its RAM marker after collecting the first session.

Native geometry/mock-renderer and existing protocol/transport tests passed. No DHU visual preview or fresh live-stream measurement preceded that installation. The packed-720 artifacts were not promoted to the checked-in baseline.

## Original plan and baseline (historical)

The remaining sections preserve the original investigation plan and its limits; current status is above.

MU1438 / Nvidia Tegra, classic layout. The original AA baseline was configured for 1280×720 at 30 fps, cropped (160,180,960,360), and scaled that map to 1440×540. Main-screen console controls worked in that configuration. The older 1080p configuration produced a sharper picture but was associated with all console controls failing. A resolution/load relationship is supported; a specific overload root cause has not been proven.

Do not change input declarations, main-channel routing, decoder policy, shared HMI ownership or metadata features as part of the first image-quality comparison.

## Stage 1 — establish where detail is lost

Record actual H.264 SPS dimensions, cadence and encoded bitrate for cluster and main display; requested settings are not sufficient evidence. Compare a decoded source frame at native size with the rendered cluster result. Use equivalent map position, UI scale, app and day/night state. Keep captures private.

Distinguish low source resolution, compression artifacts, fractional scaling and unexpected extra resampling. Also record input responsiveness, decoder waits/retries, queue occupancy and resource use. No changes are required merely to prepare this analysis offline.

## Stage 2 — preferred first experiment: better-packed 720p

Keep the coded stream 1280×720 and 30 fps. Propose a 1280×480 useful map area: zero horizontal margins, total vertical margin 240, and source crop (0,120,1280,480), displayed at the same 1440×540.

| | Current | Proposed |
| --- | --- | --- |
| Coded frame | 1280×720 | 1280×720 |
| Useful map crop | 960×360 | 1280×480 |
| Display output | 1440×540 | 1440×540 |
| Linear enlargement | 1.5× | 1.125× |
| Useful map samples | 345,600 | 614,400 |

This is 33% more samples in each direction and about 78% more useful map pixels. It is not a measured sharpness improvement yet. Recalculate DPI, real density and content/stable insets so physical UI size and placement remain consistent. It is not enough to crop a different rectangle from the old picture.

Coded dimensions and nominal decoded-surface memory stay the same. More nonblank detail may increase encoded bitrate and processing cost, so do not call it a zero-load change.

Use Google's DHU to preview the display contract offline. A DHU success validates geometry, not Tegra/NvSS throughput. A separate parked vehicle test needs explicit coordination, a verified rollback pair and unchanged input settings.

## Stage 3 — higher-resolution branch, only if needed

First determine whether the main screen actually negotiates 60 fps. Its stock configuration permits 30/60. If it runs at 60, consider a separately measured 30-fps main stream with a 1080p/30 cluster. If already 30, there is no free gain from this change.

Investigate input delivery/focus alongside decoder contention, buffer lifetimes, scheduling and queues. Require continuous main-screen controls during testing. Do not declare the Tegra incapable of 1080p based solely on the earlier failure; it already decoded the sharper stream.

Do not reduce load by arbitrarily discarding H.264 reference frames. Dropping frames after decoding does not save decode work. Arbitrary 15/20-fps negotiation is not validated in our legacy receiver.

## Secondary options

- Bitrate/scaler tuning only if source-vs-output evidence points there; no verified receiver bitrate knob is available yet.
- DPI/font-size adjustment for legibility, with explicit layout trade-offs; this does not increase physical detail.
- Arbitrary native 1440×540 AA encoding remains research, not a supported shortcut. Older schema labels suggesting resolution ranges were corrected upstream to insets.

## Acceptance / stop criteria

- Main rotary, select, back and normal app navigation all remain functional.
- Map fills the same intended classic-layout area; instructions and labels are not clipped.
- Improvement is visible in comparable source/output captures, not just larger text.
- No sustained queue growth, repeated decode timeouts, black frames or context oscillation.
- Stop/start guidance and disconnect/reconnect restore normal operation.
- Stop on a control regression; restore the verified 720p baseline before further experiments.

## Sources and limits

- [Google DHU video and cluster configuration](https://developer.android.com/training/cars/testing/dhu?hl=en): standard resolutions, margins, density and secondary-display emulation.
- [NVIDIA Tegra 3 announcement/specification](https://nvidianews.nvidia.com/news/nvidia-quad-core-tegra-3-chip-sets-new-standards-of-mobile-computing-performance-energy-efficiency-6622881): 1080p High Profile support at 40 Mbps, not a guarantee for Audi's simultaneous workloads.
- [NVIDIA Tegra 3 TRM scope](https://developer.nvidia.com/embedded/tegra-3-reference): internal video acceleration is not documented there.
- [Updated open AA additional-video definitions](https://github.com/mrmees/open-android-auto/blob/main/oaa/video/AdditionalVideoConfigData.proto): prior “resolution range” labels were corrected to display insets. This is reverse-engineered research, not a Google compatibility guarantee.
- Local implementation: `androidauto/src/native/aa_wire.h` and `aa_renderer.c`.
