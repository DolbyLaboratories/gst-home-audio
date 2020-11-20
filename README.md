# gst-home-audio

## What It Is
GStreamer plugins collection for building Dolby Home Audio media pipeline 

## Dependencies
 - [Python](https://python.org) (version 3.5 or newer)
 - [Ninja](https://ninja-build.org) (version 1.7 or newer)
 - [Meson](https://mesonbuild.com/) (version 0.54 or newer)
 - [GStreamer](https://gstreamer.freedesktop.org/) (version 1.16.2 or newer)
 - more, see building section...

## Building
On debian based systems, install dependencies:
```console
apt-get install ninja-build
pip3 install meson
```

More on installing Meson build can be found at the
[meson quick guide page](https://mesonbuild.com/Quick-guide.html).

Get other dependencies,
```console
apt-get install \
    libglib2.0-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-0 \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    libgtest-dev \
    libjson-glib-dev
```

Then configure with meson, and build with ninja
```console
meson build
ninja -C build
```

## Running
First we have to tell gstreamer where to look for the newly build plugins:
```console
export GST_PLUGIN_PATH=/path/to/your/clone/gst-home-audio/build/plugins
```

Test if gstreamer can properly retrieve information about the plugins
```console
gst-inspect-1.0 dlbdap
```

Run pipeline
```console
gst-launch-1.0 filesrc location=<file.ec3> ! dlbaudiodecbin ! dlbdap ! \
    capsfilter caps="audio/x-raw,channels=8,channel-mask=(bitmask)0xc003f" ! \
    wavenc ! filesink location=out.wav
```

DAP channel mask uses [gstreamer channel layout](https://gstreamer.freedesktop.org/documentation/audio/gstaudiochannels.html?gi-language=c#GstAudioChannelPosition), 
in the example above it is 5.1.2

Some other useful masks:
- 2.0 - 0x3
- 5.1 - 0x3f 
- 7.1 - 0xc3f 
- 2.1.1 - 0x8000b
- 5.1.2 - 0xc003f 
- 5.1.4 - 0x3303f 
- 7.1.2 - 0xc0c3f 
- 7.1.4 - 0x33c3f 

you can find full list of masks in the [DAP unit tests](tests/check/elements/dlbdap.c)
in `dap_test_negotation_tuple`.
