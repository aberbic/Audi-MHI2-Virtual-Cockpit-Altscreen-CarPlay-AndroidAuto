# Development handoff

## Baseline to preserve

- CarPlay secondary map: smooth, automatic classic-layout takeover, Apple Maps wheel zoom.
- AA: owner-accepted fuller 1080p secondary map plus main-screen 30fps-only configuration; visible resolution problem reported solved on 2026-10-02. Preserve this paired configuration. Extended controls/recovery regression testing remains pending.
- Shared context/label restoration and bounded renderer supervision.
- Original iAP2 driver restored; metadata marker absent on the last checked vehicle state.

Do not infer that every file in this repo is part of that baseline. Default bundle generation excludes both experimental directories and excludes touchpad unless explicitly requested.

## Touchpad

The binarybase reference targets MU0910/AU57x, not MU1438/AUG22. Directly replacing its classes would lose MU1438 constructors, virtual-button model IDs and input methods. The port instead edits the stock MU1438 classes and keeps native key translation.

The initial port loaded, bound to CarPlay and passed host/J9 checks, but the owner reported no touchpad response. A diagnostic-only revision was staged and verified. **Actual raw touchpad-event capture and a working mapping are still pending.** Inspect the raw DSI keyboard/encoder/gesture and direct touch-event paths before changing thresholds or IDs. Tests passing did not establish the right hardware event stream.

## Routing metadata

1. Factory navigation produced real BAP maneuver/distance/ETA updates; the parked capture showed calculating/off-road descriptors, not a normal road-turn rendering proof.
2. CarPlay navigation ownership changed, but no equivalent detailed BAP updates appeared.
3. Tegra's `mm-ipod` loads `ipod-drvr-iap2.so`; the supplied Qualcomm implementation uses Cinemo. Those attachment points differ.
4. The first proxy used immediate symbol binding and failed on optional HID-volume imports. Matching the stock loader's global/lazy behavior fixed loading; the owner then confirmed normal CarPlay.
5. Initial capability negotiation advertised request messages in the wrong receive direction. The phone's rejection identified them explicitly. Corrected directions are send 0x5200/0x5203; receive 0x5201/0x5202/0x5204.
6. The phone accepted the corrected identification. The recorder initially requested guidance before authentication and had an in-flight race that sent duplicate requests.
7. A one-shot request through the stock client API after authentication yielded real 0x5201/0x5202 messages: route state and queued maneuver descriptions/types/angles. The resource-manager mount root needed a read-only descriptor, even though requests use IPC.
8. The unit subsequently disconnected and would not reconnect. `mm-ipod` remained waiting on the USB controller process. The original driver was restored; a manual MMI restart restored normal CarPlay.

The latest sources add an authentication gate, in-flight guard and stock-identification fallback. They passed isolated tests, **not a successful live lifecycle retest**. The exact disconnect failure is unresolved; do not claim those fixes resolve it.

Next work should be offline comparison of the supplied vendor design and Luka's public implementation: subscription readiness/retries, shutdown ownership, receive callback concurrency, resource lifetime, full route deltas and active-maneuver selection. Only after that should another bounded, reversible parked test be considered. Do not publish queued instructions as the current turn without the corresponding current-route state.

Additional review found a concrete native receive-packet ownership mismatch and a direct-coordinate touchpad reference. See the [pinned repository review](REFERENCE-REVIEW-2026-10-02.md). The buffer-ownership finding raises a strong exhaustion hypothesis but does not constitute a live-validated fix.

## Contributing

Report exact train/MU, component hashes, phone OS/app, gauge layout, what changed, and whether main-screen controls remained functional. Include minimal sanitized events, not full logs or firmware. Never upload VINs, device identifiers, credentials, routes, keys, activation files or vendor packages.

Keep normal CarPlay and AA tests separate from metadata experiments. Preserve stock file backups before writes, verify uploads/readbacks, return flash read-only, and coordinate manual restarts. QNX `/tmp` is shared memory: do not assume directories or atomic renames work there.

No vehicle changes should be made merely to collect a report or check repository status. Fresh-install testing, disconnect/reconnect reliability, unit-test gaps and diagnostic ergonomics are valuable contributions alongside new features.
