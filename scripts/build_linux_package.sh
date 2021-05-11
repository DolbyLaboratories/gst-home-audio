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

mkdir -p gst-ha-flexr/bin gst-ha-flexr/lib/gstreamer-1.0
mkdir -p gst-ha-dap/bin gst-ha-dap/lib/gstreamer-1.0

find gst-home-audio/bin -name "*flexr*" -exec cp -R "{}" gst-ha-flexr/bin \;
cp -R gst-home-audio/lib gst-ha-flexr
find gst-ha-flexr/lib \( -name "*dap*" -o -name "*oar*" -o -name "*decbin*" -o -name "*typefind*" \) -delete

find gst-home-audio/bin -name "*dap*" -exec cp -R "{}" gst-ha-dap/bin \;
cp -R gst-home-audio/lib gst-ha-dap
find gst-ha-dap/lib -name "*flexr*" -delete

tar czf gst-ha-flexr.tar.gz gst-ha-flexr
tar czf gst-ha-dap.tar.gz gst-ha-dap

mv gst-ha-flexr.tar.gz "$curdir"
mv gst-ha-dap.tar.gz "$curdir"
rm -rf gst-home-audio
rm -rf gst-ha-flexr
rm -rf gst-ha-dap
