#!/bin/sh
# Host test of the MAP feature on the browser simulator sources (Linux, macOS or WSL).
# Needs clang. Frames are written as raw RGB565 (240x240) to build/host-tests/.
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/host-tests
SRC=$(find app web third_party/synth -name '*.c' -not -path 'app/board/*' | sort)
clang -std=c11 -O1 -fwrapv -fno-fast-math -ffp-contract=off -DFM1_HOST_TEST -DFM1_OPEN_WEB -w \
  -Iweb/engine -Iweb/synth -Ithird_party/synth -Isdk/include -Iapp/synth -Iapp \
  $SRC tools/host-tests/map_test.c -o build/host-tests/map_test -lm
./build/host-tests/map_test
