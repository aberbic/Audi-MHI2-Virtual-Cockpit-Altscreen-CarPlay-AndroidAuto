# Touchpad port — unfinished

Reference: [binarybase/mhi2-touchpad-fix](https://github.com/binarybase/mhi2-touchpad-fix), pinned in `NOTICE`. Its MU0910 JAR is not used. The MU1438 port preserves stock constructor signatures, model IDs, synthetic accessors and CarPlay key translation.

**Vehicle status:** loaded and bound to CarPlay, but swipes/taps did not work. The diagnostic revision was staged; its actual event capture and correct event mapping are pending. This is not a finished touchpad fix.

```sh
bash scripts/build-hmi.sh
bash touchpad/build.sh
# Expert opt-in only, after understanding the pending status:
python3 scripts/prepare-bundle.py --touchpad
```

The intended mapping is keyboard 9, raw encoder 76/77 and gestures 4/3: fast swipe emits three DPAD ticks, normal swipe one, and a tap selects after 250 ms. The hypothesis that this event stream applies to the tested Q7 is **not validated**. MU1438 also has a direct touch-event path. The recorder observes both without changing that path.

Only a bound CarPlay controller enables translation. AA/native paths and physical rotary/joystick behavior pass through in host tests. A missing gesture-end times out after 1500 ms. Real-vehicle non-regression testing is still required.

After installing the optional build and manually restarting MMI, create the empty marker `/tmp/mu1438-touchpad-trace.enabled` through your authorized shell. Capture a small set of stationary swipes/taps, then remove that marker. The bounded log `/tmp/mu1438-touchpad-input.log` contains keyboard IDs, event integers and coordinates; `/tmp/mu1438-touchpad.log` records binding/translation events. Do not publish unreviewed logs.

No iAP2 or routing-metadata change is involved. Rollback uses the same saved baseline installer snapshot.
