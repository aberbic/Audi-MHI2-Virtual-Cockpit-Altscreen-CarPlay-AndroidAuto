# CarPlay route metadata — unstable research

**Do not install this as a working integration.** The last vehicle experiment received genuine Apple Maps maneuvers but later disconnected and failed to reconnect. Stock iAP2 plus a manual MMI restart recovered it. Normal CarPlay/AA map projection does not require this adapter.

Included sources:

- `rgd_wire.*`: bounded complete-message parsing and capability construction.
- `rgd_monitor.*`: passive link/CSM framing, negotiated session tracking, checksums, fragmentation and sequence handling.
- `rgd_packet.*`: exact MU1438 packet-buffer adaptation with size checks.
- `driver_probe.c`: load/delegate to a separately preserved original driver using stock-compatible loader flags.
- `rgd_adapter.c`: opt-in capability declaration, receive observation and subscription; no BAP/display output.
- `request_guidance.c`: single request through the existing stock client API; not a general replay tool.
- Fixtures and tests for malformed inputs, the observed rejection, loader delegation, callback timing and concurrency guards.

Latest code fixes the observed early-authentication request and duplicate-send race. **Those changes are isolated-test validated only, not live lifecycle validated.** Full route state, current-maneuver selection, reliable ETA/distance delivery, teardown and disconnect/reconnect remain open.

```sh
bash experimental/metadata/test.sh
bash experimental/metadata/build.sh
```

Builds stay in ignored `out/`; they do not connect to a vehicle or modify flash. There is intentionally no public one-command experimental deployment script. The library depends on exact MU1438 internal layouts and a separately preserved original driver. Review the implementation and the [handoff](../../docs/DEVELOPMENT.md) before any future parked test. Retain an independent stock restore method.

The opt-in marker used by the code is `/mnt/app/root/mu1438-rgd/metadata.enabled`. Do not create it during normal installation. Recording may include road names and route text in `/tmp/mu1438-rgd-metadata.log`; those are private. The default package does not install this library or marker.

The [Luka reference](https://github.com/luka-dev/mib2q-carplay-rgi) documents metadata support from Apple/Google Maps and lack of it from Waze in its testing. App behavior is not interchangeable with availability of cluster map video. The Qualcomm attachment and renderer cannot simply be loaded on Tegra.
