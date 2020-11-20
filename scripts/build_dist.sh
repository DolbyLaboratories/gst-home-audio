#!/usr/bin/env bash
set -euo pipefail

curdir=$(pwd)
projroot=$(dirname "$0")
distsubproj=${1:-export}

cd "$projroot/.."
rm -rf buildtmp
mkdir buildtmp

meson buildtmp --buildtype=release -Ddistsubproj="$distsubproj"
meson dist -C buildtmp --no-tests --include-subprojects

cp buildtmp/meson-dist/* "$curdir"
rm -rf buildtmp
