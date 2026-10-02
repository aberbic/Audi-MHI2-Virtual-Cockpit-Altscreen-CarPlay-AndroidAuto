# Additional reference review — 2026-10-02

Read-only source, documentation and binary inspection. No vehicle deployment and no third-party binaries copied into this public repository. Reference checkouts are kept separately in the private workspace.

| Repository | Inspected commit | Relevant target |
| --- | --- | --- |
| [joeyQuery/MHI2-altScreen](https://github.com/joeyQuery/MHI2-altScreen) | `59b7fa6d1b81e0da266cf4f70b4cf54c34bc03dc` | Audi A4 B9 MU0678/P3241, HARMAN iAP2, LVDS |
| [chopinwong01/mhi2-android-auto-video-vc](https://github.com/chopinwong01/mhi2-android-auto-video-vc) | `1922a7e22c7aa388f2068d491646aae2b0721d98` | VW MIB2.5 High MU1367, MOST display path |
| [chefranov/mhi2-au37x-carplay](https://github.com/chefranov/mhi2-au37x-carplay) | `8968119c136b1f148486149de0950058829a2f33` | Audi A3 AU37x/P5089 MU1326, HARMAN iAP2 |

These are references, not new compatibility claims for MU1438. Documentation and binaries were cross-checked where possible; repository assertions remain author-reported results.

## 1. Chefranov: closest new lead for touchpad and metadata

The current repository distributes prebuilt artifacts, not the complete native/Java source. Its native RGI hook runs in HARMAN `mm-ipod`, unlike the Cinemo/Qualcomm integration. It therefore deserves priority for interoperability analysis, but not direct installation on another train.

### Touchpad: use the direct coordinate path

Bytecode inspection of `dpad_hook.jar` shows the CarPlay inner controller routing `updateTouchEvent(...)` and non-touchscreen `updateTouchEvents(...)` to a cursor/DPAD controller. It tracks finger position, accumulated movement, thresholds and touch end. This is materially different from the MU0910 keyboard-9 encoder/gesture path our first port used.

MU1438 has both direct touch-event methods in its stock controller. This is a strong candidate attachment point for a correct port. It is not proof the car uses that path: capture the staged diagnostic events first. The prebuilt class references `DSICarplaySafe`, whereas our MU1438 inner controller uses a different API/accessor layout. Do not replace it wholesale.

### Metadata: receive-packet ownership is a major missing piece

The native library contains receive-pool accounting and explicit reclamation of packets rejected by the stock driver. Static inspection distinguishes a return value of 1 (legitimate retained ownership) from other positive error returns. JoeyQuery's independent trace of that package reports reclamation for error 22 on route-guidance messages and a six-buffer receive pool.

Our saved MU1438 driver independently shows the relevant control flow:

1. The unextended control-message table has no 0x52 group.
2. `link_handle_reply` returns positive error 22 for an unrecognized table entry.
3. `link_handle_iap2pkt` propagates the nonzero result.
4. `transport_recv_pkt` skips returning the receive packet when the result is positive.

Our experimental adapter observes bytes and advertises 0x52 messages, but does not currently repair that ownership behavior or register those messages in the native table. Our first successful capture contained one route-state update and five maneuver updates, then stopped.

**Assessment:** the static mismatch is confirmed; receive-pool exhaustion is now a strong explanation for the observed six-message stall. The exact pool occupancy was not captured on our unit, and this is not yet a confirmed explanation for every USB/disconnect symptom. No fix has been implemented or vehicle-tested in this review.

Before another metadata test, investigate native message recognition and packet ownership at the correct boundary. Do not indiscriminately return all retained packets: return value 1 can transfer ownership legitimately, and double-returning buffers could corrupt the stock driver. Add ownership/lifetime tests and bounded pool diagnostics independently of rendering.

The October 1 package uses a separate live maneuver renderer and precompiled shader blobs. Older cached documentation described thousands of PNG frames; that is not the inspected commit. Shader handling is a potential rendering reference, not a reason to replace our working NvSS map path.

Sources: [pinned package README](https://github.com/chefranov/mhi2-au37x-carplay/blob/8968119c136b1f148486149de0950058829a2f33/README.md), static inspection of its `dpad_hook.jar` and `librgd_hook.so`, and the locally backed-up MU1438 driver. No binary contents are redistributed here.

## 2. JoeyQuery: close Audi LVDS architecture and a lifecycle warning

The author reports working CarPlay map projection, separate NvSS renderer ownership, wheel zoom and two dial-layout safe areas on MU0678. Particularly useful leads:

- Layout detection from the native map zoom-area announcement rather than assuming a fixed gauge layout.
- A measured sender-side frame-rate effect from display feature/HID advertisement. Measure our actual cadence before assuming it applies to our iPhone/firmware.
- A native DSI tracing route that can help identify real input/layout events.
- Separation of map video, HUD/BAP instructions and cluster maneuver-tile presentation.

The guidance experiment uses Chefranov's HARMAN hook with Java adaptations. It was **rolled back** after an unexplained route-end CarPlay drop. The trace also shows an HMI method mismatch. This is useful comparative evidence, not a proven stable solution to our disconnect issue, and the same cause must not be assumed.

Its map video pipeline is a closer conceptual match than a MOST/Qualcomm implementation, but its firmware, visible area and display composition differ. Never transplant its context/layer numbers or whole JARs without validation.

Sources: [status](https://github.com/joeyQuery/MHI2-altScreen/blob/59b7fa6d1b81e0da266cf4f70b4cf54c34bc03dc/STATUS.md), [guidance findings](https://github.com/joeyQuery/MHI2-altScreen/blob/59b7fa6d1b81e0da266cf4f70b4cf54c34bc03dc/docs/hud-route-guidance.md), [controls](https://github.com/joeyQuery/MHI2-altScreen/blob/59b7fa6d1b81e0da266cf4f70b4cf54c34bc03dc/docs/cluster-controls.md).

## 3. Chopinwong: AA backpressure and main-screen isolation

The current configuration/source uses an 800×480 secondary stream, FFmpeg software decoding and a MOST/OpenKODE/GLES display path. Hardware NvSS/NvMedia decoding is listed as unfinished. Its 800×480-only statements describe that implementation/target; they are not a universal AA limitation and do not override our observed 720p/1080p streams.

The strongest reusable idea is end-to-end pacing: the phone's frame credit is released after the player reports presentation. Our AA adapter currently acknowledges reception/queueing without waiting for the renderer. Compare these policies when investigating load, queue growth and the 1080p control regression.

Do not blindly copy a one-credit presentation policy into an asynchronous decoder: this project documents how frame buffering and withheld credit can deadlock each other. Define the NvSS acceptance/completion boundary and bounded credit window first. Keep this separate from the initial 720p geometry A/B test.

Other useful areas are independent secondary sink/renderer state, main-stream telemetry, startup focus sequencing and the companion project's directional-key zoom semantics. Their claimed 2 MB socket buffers do not match what our earlier QNX tests accepted, so transport limits must be measured on our firmware.

Source: [pinned hook implementation](https://github.com/chopinwong01/mhi2-android-auto-video-vc/blob/1922a7e22c7aa388f2068d491646aae2b0721d98/src/video_sink_hook.c), [player](https://github.com/chopinwong01/mhi2-android-auto-video-vc/blob/1922a7e22c7aa388f2068d491646aae2b0721d98/player/opengl_gpu.cc), [configuration](https://github.com/chopinwong01/mhi2-android-auto-video-vc/blob/1922a7e22c7aa388f2068d491646aae2b0721d98/scripts/gal_dualscreen.conf.example).

## Resulting priorities

1. AA image quality: the review initially supported starting with better-packed 720p. Subsequent local testing found only a slight improvement; the owner then accepted fuller 1080p with the main-screen 30fps cap. See the updated [test record](AA-IMAGE-QUALITY-PLAN.md).
2. Touchpad: capture/direct-coordinate mapping before further keyboard-ID guesses.
3. Metadata: resolve native receive-packet ownership before any further live driver test.
4. AA performance research: renderer-aware backpressure and independent primary-channel telemetry, as a separate experiment.
5. Preserve firmware-specific checks, licence boundaries and the working rollback baseline. No third-party installer or binary was run on the car for this review.
