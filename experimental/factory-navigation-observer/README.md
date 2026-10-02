# Factory navigation observer

Records existing Audi BAP navigation service calls without altering them. Text is represented by length; capture is opt-in and capped at 4,000 events per HMI process. This recorder successfully captured factory route activation, maneuver descriptors, distance, ETA and visibility updates on MU1438.

It does **not** extract iPhone metadata, implement a guidance bridge, or prove that maneuver graphics coexist with context 900. The parked factory capture used calculating/off-road states rather than a road-turn visual test.

Build with `bash experimental/factory-navigation-observer/build.sh` after the baseline HMI build. Output is private because it includes an additional modified OEM class. No installer is provided; integrate only as a deliberate diagnostic build with backups and a manual HMI restart.

The recorder is off unless `/tmp/mu1438-nav-observe.enabled` exists. Its output is `/tmp/mu1438-nav-observe.log`. A reboot clears the RAM marker. Source tests verify redaction, bounded capture and preservation of input data and the baseline JAR entries.
