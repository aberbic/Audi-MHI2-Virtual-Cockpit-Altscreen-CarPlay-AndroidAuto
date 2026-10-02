# Build prerequisites

Use a current host JDK for ASM patch tooling, Java 8 for J9-compatible helper classes, Python 3, a host C compiler and Docker. All private inputs and generated artifacts stay in ignored directories.

## Native toolchain

The known build used [luka-dev/qnx65-armv7-toolchain](https://github.com/luka-dev/qnx65-armv7-toolchain), commit `56a66557245af14077678cd28a83ce3a337d9e2d`, image tag `mhi2-qnx65-armv7-public:4.9`. Follow the upstream setup with SDK/compiler material you are entitled to use; this repository does not ship it.

The original image was built locally with:

```sh
docker buildx build --platform=linux/amd64 --target base-env \
  --build-arg BASE=base-4.9 --load -t mhi2-qnx65-armv7-public:4.9 .
```

Run that in the toolchain checkout, after its prerequisites. Old Debian base-image package URLs may need the upstream snapshot/mirror maintenance fix; do not disable package signature verification.

To rebuild native runtimes:

```sh
bash carplay/scripts/build-native.sh
bash androidauto/scripts/build-native.sh
```

Outputs go under each component's `out/`. They do not replace bundled, owner-tested `bin/` files. `prepare-bundle.py` intentionally packages the checked-in baseline binaries. If developing replacements, validate them separately and update the artifact manifest deliberately.

CarPlay flags: `MU1438_PASSIVE=0`, `MU1438_AUDI_GEOMETRY=1`, `MU1438_RUNTIME=3`.
AA flags: `AA_LIVE=1`, `AA_CLASSIC_LAYOUT=1`, **`AA_HD=0`**, `AA_ZOOM=1`, `AA_RELEASE=1`. The zoom-capable code advertises input capabilities, but forwarding remains opt-in and was not functional in the tested AA app. Do not change input declarations as part of a resolution-only comparison.

## HMI reconstruction

Convert your exact original JXE with [luka-dev/jxe2jar](https://github.com/luka-dev/jxe2jar), reference commit `9eeb45bbf14bf8afe3452c7be96a4d1f0206a286`. Follow its conversion instructions and put the resulting JAR at `inputs/mu1438-stock.jar`. The original `inputs/lsd.jxe` must match the profile even if conversion metadata varies.

Also place these standard build dependencies in `inputs/`, obtained from their publishers:

- `asm-9.7.jar`
- `asm-tree-9.7.jar`
- `org.osgi.framework-1.10.0.jar`

```sh
bash scripts/build-hmi.sh
bash scripts/build-tools.sh
```

The HMI generator modifies only the documented methods in two exact stock classes and adds the shared bridge. It emits Java 1.4-compatible helpers and preserves the native methods outside those changes. Generated JARs contain OEM-derived classes: keep them private.

## Optional / experimental

```sh
bash touchpad/build.sh               # unfinished touchpad port + diagnostics
bash experimental/metadata/test.sh   # host protocol/transport tests
bash experimental/metadata/build.sh  # QNX recorder; DO NOT deploy as baseline
bash experimental/factory-navigation-observer/build.sh
```

No experimental build script connects to a car. Metadata recorder outputs must not be treated as a working release merely because compilation/tests pass: that happened during development and did not establish lifecycle safety.

## Public-package checks

```sh
python3 scripts/verify-public.py
bash scripts/test.sh
```

The validator checks the baseline binary hashes and rejects common private/generated artifacts from the publishable tree. It is a safeguard, not a substitute for reviewing `git diff --cached` and secret scanning before publishing.
