#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
task=experimental/factory-navigation-observer
dest=out/factory-observer
test -s out/hmi/mu1438-cluster.jar
mkdir -p "$dest/classes" "$dest/tools"
docker run --rm -v "$PWD:/w" -w /w eclipse-temurin:8-jdk-jammy \
 javac -source 1.4 -target 1.4 -Xlint:-options -cp inputs/mu1438-stock.jar -d "$dest/classes" "$task/NativeNavObserver.java"
javac -cp inputs/asm-9.7.jar:inputs/asm-tree-9.7.jar -d "$dest/tools" "$task/PatchNavObserver.java"
java -cp "$dest/tools:inputs/asm-9.7.jar:inputs/asm-tree-9.7.jar" PatchNavObserver \
 inputs/mu1438-stock.jar out/hmi/mu1438-cluster.jar "$dest/classes/local/mu1438/NativeNavObserver.class" "$dest/mu1438-cluster-observer.jar"
javac -cp "$dest/classes:inputs/mu1438-stock.jar" -d "$dest/tools" "$task/ObserverTest.java"
java -cp "$dest/tools:$dest/classes:inputs/mu1438-stock.jar" ObserverTest out/hmi/mu1438-cluster.jar "$dest/mu1438-cluster-observer.jar"
