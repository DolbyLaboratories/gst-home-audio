#!/usr/bin/env bash
set -euo pipefail

curdir=`pwd`
testroot="$1"
projroot=$(dirname "$0")

if test "$testroot" = ""; then
	echo "Test root is missing" >&2
	exit 1
fi

if test $(uname -m) != "x86_64"; then
	echo "Unsupported DUT arch" >&2
	exit 1
fi

cd "$projroot/.."
rm -rf buildtmp
mkdir buildtmp

meson buildtmp --buildtype=release --prefix=/tmp/gst-home-audio --libdir=lib --bindir=bin --strip
ninja -C buildtmp install

rm -rf buildtmp
cd /tmp/

mkdir -p gst-ha-dap/bin gst-ha-dap/lib/gstreamer-1.0

find gst-home-audio/bin -name "*dap*" -exec cp -R "{}" gst-ha-dap/bin \;
cp -R gst-home-audio/lib gst-ha-dap
find gst-ha-dap/lib -name "*flexr*" -delete

cd "$curdir"
cp -r /tmp/gst-ha-dap/* "$testroot"/Source_Code/dut/linux64/
rm -rf /tmp/gst-home-audio
rm -rf /tmp/gst-ha-dap
