#!/usr/bin/env bash
set -euo pipefail

curdir=`pwd`
projroot=$(dirname "$0")

cd "$projroot/.."
rm -rf buildtmp
mkdir buildtmp

meson buildtmp --buildtype=release --prefix=/tmp/gst-home-audio --libdir=lib --bindir=bin --strip
ninja -C buildtmp install

rm -rf buildtmp
cd /tmp/

tar czf gst-home-audio.tar.gz gst-home-audio

mv gst-home-audio.tar.gz "$curdir"
rm -rf gst-home-audio
