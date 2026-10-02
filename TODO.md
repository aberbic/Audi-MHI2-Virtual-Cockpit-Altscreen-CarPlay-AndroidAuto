# Project to-do

Planning only. These items do not authorize vehicle changes or establish compatibility with other firmware.

## AA sharpness — owner accepted 2026-10-02

- [x] Follow the [saved AA image-quality plan](docs/AA-IMAGE-QUALITY-PLAN.md) through an owner-accepted resolution fix.
- [ ] Verify received SPS dimensions and actual cadence on both video streams.
- [ ] Compare source-frame sharpness with final cluster output.
- [x] Prototype a 1280×480 useful map area inside the existing 1280×720 stream.
- [x] Recalculate margins, crop, DPI and safe-area insets together; verify physical-layout equivalence in tests.
- [x] First parked packed-720p test: actual geometry confirmed; owner reports only a slight improvement, not satisfactory final quality.
- [x] Validate offline layout and complete a separate parked fuller-1080p/main-30fps test. Owner reports the resolution problem solved.
- [x] Promote the exact tested pair and main-screen configuration to the baseline; preserve source, tests and recovery instructions.
- [ ] Complete extended rotary/select/back, smoothness, ownership and reconnect regression checks; quality acceptance alone does not prove all of these.
- [ ] Isolate the effect of the main-screen 30fps limit only if further investigation is needed; the original negotiated main rate was not measured.

## Touchpad

- [ ] Capture actual MU1438 raw DSI and direct touch events with the staged diagnostic build.
- [ ] Validate keyboard/event IDs and input ownership; do not assume the MU0910 mapping.
- [ ] Compare Chefranov's direct-coordinate touch-event bridge with MU1438's corresponding methods.
- [ ] Verify physical rotary/joystick and AA input are unchanged.

## CarPlay guidance metadata

- [ ] Investigate the disconnect/reconnect failure offline before another driver experiment.
- [ ] Compare Tegra-specific integration and lifecycle handling in the reference projects.
- [ ] Resolve the native positive-error/retained-packet ownership mismatch; investigate the six-message stall before another live test.
- [ ] Validate the authentication/in-flight fixes on a separately approved parked test.
- [ ] Capture complete route state and current-maneuver selection, not just queued maneuvers.
- [ ] Add BAP/native presentation only after transport lifetime and restoration are reliable.

## CarPlay album art on the Virtual Cockpit

- [ ] Use [Chefranov's working MU1326 implementation](https://github.com/chefranov/mhi2-au37x-carplay) as the primary reference; adapt to MU1438 rather than installing its replacement JAR unchanged.
- [ ] Trace iAP2 now-playing artwork extraction, image conversion and the cluster's picture-request/response path offline.
- [ ] Check whether the cluster's `Picture_Upload_Download` adaptation is enabled, as required by the reference implementation; do not change coding without approval.
- [ ] Validate MU1438 HMI picture-provider interfaces and integrate without disturbing existing track text, media controls or AltScreen ownership.
- [ ] Test track changes, missing artwork, reconnects and stale-image cleanup with bounded memory/cache use.
- [ ] Keep private artwork/captures and proprietary reference binaries out of the public repository.

## Other open work

- [ ] Investigate AA cluster zoom semantics independently of video changes.
- [ ] Validate more gauge layouts, day/night behavior and long-duration recovery.
- [ ] Fresh-install and rollback validation of the public SSH/M.I.B. package.
- [x] Review the three additional reference repositories; [pinned findings](docs/REFERENCE-REVIEW-2026-10-02.md).
- [ ] Evaluate renderer-aware AA credit/backpressure independently of the geometry experiment.
