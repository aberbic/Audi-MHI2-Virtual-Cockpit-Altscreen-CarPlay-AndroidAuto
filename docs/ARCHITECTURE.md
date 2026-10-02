# Architecture

```text
Phone secondary video
  -> CarPlay hook / AA secondary endpoint
  -> bounded, complete-access-unit loopback transport
  -> Tegra NvSS decoder -> layer 60 / context 900 -> cluster LVDS display 4
                                    |
                          fresh renderer heartbeat
                                    |
                    shared HMI context + native-label gate
```

The centre screen remains under the original projection implementation. The cluster panel is 1440×542; the selected video area is 1440×540. This Audi's map transport is LVDS, not the reference Škoda MOST/remux path. Qualcomm OMX/libscreen renderers are not portable to Tegra.

## CarPlay

The hook advertises a secondary display and handles stream 111. The source canvas is 1440×540, with classic-layout safe area 400×297 at (520,87) and physical size 320×120 mm.

Hook-first `DT_NEEDED` order matters for the tested `/info` and AES interposition. The integrator ignored arbitrary `LD_PRELOAD` entries in its per-child JSON, so a small, hash-locked loader delta is applied locally. The original executable is never distributed.

Loopback TCP 19820 carries AU11 framing: 8-byte header (`AU11` + network-order length), followed by one complete Annex-B access unit. Decoder initialization precedes connection so the first keyframe is retained. `TCP_NODELAY` fixed a small-header/delayed-ACK stall. A separate raw diagnostic mirror exists on 19821; do not publish captures.

## Android Auto

Stock service registration and serialization run first. The adapter adds its own video/input services (19/20) after stock serialization because this old receiver's protobuf schema drops newer cluster fields on reserialization. Negotiated channel IDs are recorded rather than universally assumed.

The default is 1920×1080 H.264 at an advertised 30 fps. Crop (0,180,1920,720) downsamples to the 1440×540 output; DPI and insets preserve the original logical UI size. The main AA frame-rate list is limited to 30 fps by a paired configuration change. The owner accepted this profile on 2026-10-02 as resolving the blurry image. An older 1080p profile was associated with failed centre-console controls; because geometry and main-frame-rate limits changed together, the precise cause remains unproven. Input-service declarations are unchanged.

TCP 19840 carries the same complete-access-unit framing. NvSS uses a 35 ms input-buffer wait, bounded timeout retries and an absent-timestamp sentinel derived from the stock wrapper. No global interception of the receiver's main media router is used.

## Shared HMI and ownership

`ClusterGate` redirects only navigation contexts 72–77 on cluster terminal 1 to custom context 900 while a renderer heartbeat is fresh. Main terminal 0 and non-navigation contexts pass through. Native map planes must not coexist with the custom plane: they occluded it during testing.

The bridge caches native road/turn/scale values, suppresses their publication while projection owns the map, shows a backend label, and restores cached values afterward. AA writes an `Android Auto` marker; the original CarPlay PID-only marker selects `CarPlay`. Wheel steps use MZ01 packets over UDP 19822. UDP 19823 is the shared renderer singleton.

The two backends are mutually exclusive in this design. No simultaneous-phone cluster arbitration is claimed. Main gauges, warning services and altitude are not intentionally modified.

## Instructions are separate from video

The factory path is `ClusterService -> CombiBAPListener -> CombiBAPServiceNavi -> BAP navigation messages`, with distinct maneuver, road, distance, ETA, lane and visibility updates. On this firmware, the combined `updateManeuverDescriptorAndExitView()` entry is a no-op; separate calls matter.

The stock Tegra iAP2 module lacked route-guidance capability advertisement. An experimental adapter received real Apple Maps maneuvers after adding it, but disconnect/reconnect failures made it unsuitable for installation. No metadata-to-BAP bridge is enabled by the stable package. Native instruction presentation also has separate visibility/composition behavior; a BAP data interface alone does not prove graphics will coexist with the custom map.
