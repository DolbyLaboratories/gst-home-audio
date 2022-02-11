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

rm -rf flexr-build
rm -rf dap-build

meson flexr-build --buildtype=release --prefix=/tmp/gst-ha-flexr --libdir=lib --bindir=bin --strip -Doar=disabled -Ddap=disabled -Daudiodecbin=disabled
ninja -C flexr-build install

rm -rf flexr-build

meson dap-build --buildtype=release --prefix=/tmp/gst-ha-dap --libdir=lib --bindir=bin --strip -Dflexr=disabled
ninja -C dap-build install

rm -rf dap-build

cd "$curdir"
cp -r /tmp/gst-ha-flexr/* "$testroot"/Source_Code/dut/gst-ha-flexr/linux64/
cp -r /tmp/gst-ha-dap/* "$testroot"/Source_Code/dut/gst-ha-dap/linux64/
rm -rf /tmp/gst-home-audio
rm -rf /tmp/gst-ha-dap
rm -rf /tmp/gst-ha-flexr
