#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
for f in mu1438-stock.jar asm-9.7.jar asm-tree-9.7.jar org.osgi.framework-1.10.0.jar; do
 test -s "inputs/$f" || { echo "Missing private build input: inputs/$f" >&2;exit 1; }
done
mkdir -p out/hmi/classes out/hmi/tools out/hmi/tests
docker run --rm -v "$PWD:/w" -w /w eclipse-temurin:8-jdk-jammy \
 javac -source 1.4 -target 1.4 -Xlint:-options -cp inputs/mu1438-stock.jar:inputs/org.osgi.framework-1.10.0.jar \
 -d out/hmi/classes hmi/src/local/mu1438/ClusterGate.java
javac -cp inputs/asm-9.7.jar:inputs/asm-tree-9.7.jar -d out/hmi/tools hmi/tools/PatchCluster.java
java -cp out/hmi/tools:inputs/asm-9.7.jar:inputs/asm-tree-9.7.jar PatchCluster inputs/mu1438-stock.jar out/hmi/classes
jar cf out/hmi/mu1438-cluster.jar -C out/hmi/classes .
javac -cp out/hmi/mu1438-cluster.jar:inputs/mu1438-stock.jar:inputs/org.osgi.framework-1.10.0.jar -d out/hmi/tests hmi/tests/GateTest.java
java -Xverify:all -cp out/hmi/tests:out/hmi/mu1438-cluster.jar:inputs/mu1438-stock.jar:inputs/org.osgi.framework-1.10.0.jar GateTest
