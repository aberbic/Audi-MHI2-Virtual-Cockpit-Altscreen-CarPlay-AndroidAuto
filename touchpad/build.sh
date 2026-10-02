#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test -s out/hmi/mu1438-cluster.jar || { echo 'Build the baseline HMI first.';exit 1; }
mkdir -p out/touchpad/classes out/touchpad/tools out/touchpad/tests
docker run --rm -v "$PWD:/w" -w /w eclipse-temurin:8-jdk-jammy \
 javac -source 1.4 -target 1.4 -Xlint:-options -cp inputs/mu1438-stock.jar -d out/touchpad/classes touchpad/TouchpadBridge.java
javac -cp inputs/asm-9.7.jar:inputs/asm-tree-9.7.jar -d out/touchpad/tools touchpad/PatchTouchpad.java
java -cp out/touchpad/tools:inputs/asm-9.7.jar:inputs/asm-tree-9.7.jar PatchTouchpad \
 inputs/mu1438-stock.jar out/hmi/mu1438-cluster.jar out/touchpad/classes out/touchpad/mu1438-cluster-touchpad.jar
javac -cp out/touchpad/mu1438-cluster-touchpad.jar:inputs/mu1438-stock.jar -d out/touchpad/tests touchpad/TouchpadTest.java
java -Xverify:all -cp out/touchpad/tests:out/touchpad/mu1438-cluster-touchpad.jar:inputs/mu1438-stock.jar TouchpadTest out/hmi/mu1438-cluster.jar out/touchpad/mu1438-cluster-touchpad.jar
