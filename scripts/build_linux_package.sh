#!/usr/bin/env bash
set -euo pipefail

curdir=`pwd`
projroot=$(dirname "$0")	    

cd "$projroot/.."

rm -rf flexr-build
rm -rf dap-build

meson flexr-build --buildtype=release --prefix=/tmp/gst-ha-flexr --libdir=lib --bindir=bin --strip -Doar=disabled -Ddap=disabled -Daudiodecbin=disabled
ninja -C flexr-build install

rm -rf flexr-build

meson dap-build --buildtype=release --prefix=/tmp/gst-ha-dap --libdir=lib --bindir=bin --strip -Dflexr=disabled
ninja -C dap-build install

rm -rf dap-build

cd /tmp/

tar czf gst-ha-flexr.tar.gz gst-ha-flexr
tar czf gst-ha-dap.tar.gz gst-ha-dap

mv gst-ha-flexr.tar.gz "$curdir"
mv gst-ha-dap.tar.gz "$curdir"
rm -rf gst-home-audio
rm -rf gst-ha-flexr
rm -rf gst-ha-dap
