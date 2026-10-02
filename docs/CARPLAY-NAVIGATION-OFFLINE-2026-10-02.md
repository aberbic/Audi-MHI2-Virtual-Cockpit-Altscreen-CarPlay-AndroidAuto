# CarPlay navigation instructions — offline findings

2026-10-02, after acceptance of the AA resolution fix. Scope: inspect saved MU1438 firmware, private logs, current project sources and pinned reference implementations. No vehicle connection, driver deployment or runtime changes.

## Outcome

The saved driver log provides direct evidence of receive-pool starvation after the guidance messages. This strengthens the earlier static ownership diagnosis substantially. It does not establish the cause of every subsequent USB/reconnect failure. The MU1438 native presentation interfaces exist, but reliable reception, a route-state cache and overlay composition are separate unfinished stages.

The desired separation is:

```text
CarPlay AltScreen video ───────────────────────> existing map renderer/ownership
iAP2 guidance -> bounded route-state cache ─┬──> native maneuver/distance/text
                                          └──> optional maneuver-card graphics
```

Ending guidance must clear its instructions, not withdraw an otherwise live CarPlay map.

## 1. Stronger evidence for the six-message stall

Private capture `rgd-disconnect-failure.d3uVfH/iap2-private.log` shows:

- Line 4899: the first route-state message; line 4900: stock dispatch reports an invalid packet.
- Lines 6154–6167: five maneuver messages, each followed by the same stock rejection.
- Line 6169 onward: repeated `packet_recvlist_pop: No packets in receive list, need to wait 0`.

The companion metadata log contains one route update and five maneuver updates. Stock traffic continues between the initial route update and the later maneuver burst; the pool-empty reports begin after that burst. Raw packets, road/location data and captures remain private and are not included here.

The exact stock driver fingerprint is already in `profiles/mu1438.json`; the inspected copy matches it. Its control flow explains the observed starvation:

| Boundary | Observed behavior |
| --- | --- |
| `link_handle_reply` | An unrecognized control-message entry produces positive error 22. A matched synchronous reply can return 1 after transferring packet ownership to its waiter. |
| `link_handle_iap2pkt` | Propagates a nonzero reply-handler result before normal control-message handling. |
| `transport_recv_pkt` | Does not return a packet to the receive pool when that internal result is positive. It then returns zero to its caller. |
| Our `receive_packet` wrapper | Delegates to that outer function; it cannot see the internal 22 or recover the packet pointer from the returned zero. |

Consequences for a future repair:

- Normalizing the outer callback's return code is insufficient.
- Observing bytes in the USB read callback does not repair native packet ownership.
- A legitimately retained reply must remain retained. Returning every packet would risk double returns/use-after-release.
- Chefranov's native implementation handles ownership at the receive/dispatch boundary and distinguishes retained replies from positive errors. That is an architectural reference, not a verified drop-in repair for this firmware.
- Pool initialization depends on negotiated parameters; do not claim a universal hard-coded six-buffer pool based solely on this trace. The exact live occupancy was not instrumented.

Next transport investigation: establish a narrowly scoped recognition/ownership contract for supported guidance messages, with explicit unknown-message and synchronous-reply behavior. Validate the contract using an isolated ownership model before writing an adapter change. Keep authentication, subscription concurrency and session reset tests separate.

## 2. Captured maneuver records are not the current instruction

The only recorded route-state update contains state zero. The five maneuver messages populate indexes 0 through 4; there is no subsequent captured current-maneuver list, live distance or ETA. The last received record cannot safely be displayed as the next turn.

Our `rgd_wire` parser is intentionally a recorder: it retains numeric values and the lengths of text/list fields, not the strings or ordered maneuver-index list needed for presentation. A separate bounded state reducer is required.

The pinned Luka protocol implementation identifies:

| Guidance data | Presentation responsibility |
| --- | --- |
| Route-level state and current-maneuver list | Decide whether guidance is active and which cached maneuver is current. |
| Indexed maneuver details | Supply type, road text, exit information and junction geometry; receipt alone does not activate a turn. |
| Distance/time deltas | Update existing state without clearing fields absent from that delta. |
| Signed exit angle | Decode as signed before mapping direction; the captured unsigned diagnostic value 65446 represents -90 in a 16-bit signed interpretation. |
| Route/session generation | Invalidate stale indexes and text when routes or phone sessions change. |

Offline acceptance cases should cover out-of-order details/list updates, partial deltas, explicit empty lists, missing current slots, rerouting, route end, reconnect, signed angles, oversized text and unsupported sources. Do not publish partial malformed updates or invent a current maneuver when data is incomplete.

## 3. MU1438 presentation interfaces are present

Inspection of the original MU1438 HMI confirms the methods used for these basic outputs:

| Output | Available interface |
| --- | --- |
| Native maneuver descriptors | `CombiBAPServiceNavi.updateManeuverDescriptor(...)` |
| Distance and approach progress | `updateDistanceToNextManeuver(int, int, boolean, int)` |
| Road/sign text | `updateCurrentPositionInfo(String)`, `updateTurnToInfo(String, String)` |
| Destination distance / time | `updateDistanceToDestination(...)`, `updateTimeToDestination(...)` |
| Route and maneuver state | `updateRGStatus(int)`, `updateActiveRGType(int)`, `updateManeuverState(int)` |
| Cluster route-info path | `ClusterService.updateNextManeuver(RouteInfoElement)` |

The factory-navigation capture independently contains descriptor, distance, route-state and destination-time calls. It is not a captured normal-road turn proof: the stationary route included calculating/off-road data.

The route-info path is conditional: MU1438's `updateNextManeuver` stores the element and calls `updateKOMOFollowInfo`. That method requires the KOMO service and follow mode; the two-element route-info publication also depends on feature availability. A matching method signature alone does not establish visible output in our classic layout.

Unlike the other reference vehicle's reported synchronization mismatch, MU1438 has the inspected `getFunctionSynchronizationHandler()` return type and `startSync(int)` method. That one mismatch is not established here. This does not establish compatibility of the rest of the foreign JAR.

## 4. Preserve the working map and address composition explicitly

`carplay/src/native/cluster_runtime.c` declares context 900 with only layer 60. `ClusterGate` selects it while the map renderer heartbeat is live; it also suppresses factory street/turn text and scale updates.

Therefore:

- Native BAP instructions and a graphical maneuver card must not be treated as the same output. Chefranov's current package supplies a separate graphical maneuver renderer as well as the native-data bridge.
- A correct native publication does not prove its graphical card is included in context 900. Determine which visual elements are cluster-native and which require an additional head-unit layer before changing composition.
- The shared gate needs an explicit owner for CarPlay guidance fields. Simply issuing competing text updates would let subsequent OEM updates blank them again.
- Keep map ownership controlled by the live AltScreen stream, not route state. Route-end cleanup must not revive the old Audi/CarPlay context oscillation.
- Metadata failure must withdraw its own stale instructions and preserve normal CarPlay/AA operation. Do not cancel a factory route as a side effect of experimentation.

## Offline implementation follow-up

`experimental/metadata/rgd_state.*` now implements the bounded state-cache prototype, with synthetic tests in the existing host suite. It handles out-of-order details/current lists, partial updates, explicit clearing, signed angles, bounded UTF-8, cache capacity and caller-provided session/route epochs. Tests include 20,000 generated byte inputs and 1,000 structured sequences. The full macOS suite and Linux metadata suite passed; the reducer also compiled with the QNX toolchain.

`test_ownership_model.py` adds abstract ownership invariants for dispatch, observers and reply waiters. It does not emulate the native bug, map native error codes or implement a driver repair. Ambiguous ownership is left unresolved rather than guessed.

Neither addition is wired into `rgd_adapter.c`, the native build script, a publisher or the installation bundle. Map binaries are unchanged. Freshness/timeout policy, lifecycle-derived epochs, units/sentinels, maneuver mapping, lane guidance and display composition remain open. See the [prototype limits](../experimental/metadata/README.md#offline-state-prototype).

## Remaining offline milestones

1. Resolve the native ownership boundary against the abstract contract, including retained replies and session transitions. No live callback repair yet.
2. Extend the tested reducer with a lifecycle/freshness controller and presentation policies using synthetic fixtures; no firmware, private road data or live sockets required.
3. Check a small MU1438-specific publisher against exact HMI signatures using mocks; define startup, field ownership and restoration before integrating.
4. Investigate instruction-card composition separately from data delivery. Reuse existing map rendering/ownership wherever possible.
5. Only after those pass: plan a parked, bounded metadata-only test with no display publication first. It must demonstrate sustained traffic and reconnect recovery before enabling presentation.

## References and limits

- [Chefranov pinned package](https://github.com/chefranov/mhi2-au37x-carplay/blob/8968119c136b1f148486149de0950058829a2f33/README.md): prebuilt MU1326 implementation, separate renderer and app-support claims. Local native/Java artifacts were inspected, not executed or redistributed.
- [Luka pinned protocol map](https://github.com/luka-dev/mib2q-carplay-rgi/blob/72d321cc9f57bc2f3a1bb76021404c96b300f326/docs/rgd/rgd-tlv.md) and source: semantic reference, not an official Apple specification or a Tegra adapter.
- [JoeyQuery guidance findings](https://github.com/joeyQuery/MHI2-altScreen/blob/59b7fa6d1b81e0da266cf4f70b4cf54c34bc03dc/docs/hud-route-guidance.md): useful separate evidence and lifecycle warnings, not proof that its shutdown failure applies to ours.
- Our exact stock MU1438 driver/HMI, private captures, `experimental/metadata`, `ClusterGate` and `cluster_runtime` sources.

Apple Maps is the appropriate first validation app: we already captured its messages. The inspected reference reports no Waze guidance metadata; Waze AltScreen map availability does not establish a maneuver feed, and future app behavior must be measured rather than assumed.
