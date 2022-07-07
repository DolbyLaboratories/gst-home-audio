# gst-home-audio

## What It Is
GStreamer plugins collection for building Dolby Home Audio media pipeline.

> :warning: Plugins require proprietary libraries to initialize properly.

## Dependencies
 - [Python](https://python.org) 3.5 or later
 - [Ninja](https://ninja-build.org) 1.7 or later
 - [Meson](https://mesonbuild.com/) 0.58 or later
 - [GStreamer](https://gstreamer.freedesktop.org/) 1.16.2 or later
 - more, see building section...

## Building

### Linux
On Debian-based systems, install the following dependencies:
```console
$ apt-get install ninja-build
$ pip3 install meson
```

More on installing Meson build can be found at the
[Meson quickstart guide](https://mesonbuild.com/Quick-guide.html).

Get other dependencies.
```console
$ apt-get install \
    libglib2.0-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-0 \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    libjson-glib-dev
```

Then configure with Meson, and build with Ninja.
```console
$ meson build
$ ninja -C build
```

### Windows
Support for Windows is enabled via [MSYS2](https://www.msys2.org/). Follow
instructions on [MSYS2 install page](https://www.msys2.org/#installation).
You should have the gcc toolchain on MSYS environment.

**Note:** *Run `MSYS2 MinGW 64-bit`.*

Install Meson and Ninja
```console
$ pacman -S mingw-w64-x86_64-meson mingw-w64-x86_64-ninja
```

Install the GStreamer dependencies (required to build plugins):
```console
$ pacman -S mingw-w64-x86_64-gstreamer mingw-w64-x86_64-gst-plugins-base \
    mingw-w64-x86_64-gst-plugins-good mingw-w64-x86_64-json-glib \
    mingw-w64-x86_64-gst-python
```

Install some useful plugins (optional)
```console
$ pacman -S mingw-w64-x86_64-gst-plugins-bad mingw-w64-x86_64-gst-plugins-ugly \
    mingw-w64-x86_64-gst-libav
```

Build and install
```console
$ meson build
$ ninja -C build
$ ninja -C build install
```

### macOS
You can install GStreamer dependencies via [Homebrew](https://brew.sh/) or you
can use [installer](https://gstreamer.freedesktop.org/data/pkg/osx/) from
GStreamer project.

```console
$ brew install gstreamer gst-plugins-base gst-plugins-good gst-python json-glib
```

Install additional Python dependencies
```console
pip3 install pycairo
pip3 install PyGObject
```

Build
```console
$ meson build
$ ninja -C build
```

## Building from source
GStreamer and the plugins can be built from source by using
[gst-build](https://gitlab.freedesktop.org/gstreamer/gst-build), or starting
from GStreamer 1.20, [GStreamer mono repo](https://gitlab.freedesktop.org/gstreamer/gstreamer).

Install `meson` and `ninja` as instructed in the [Linux](#linux),
[Windows](#windows), and [macOS](#macos) sections above.

On Linux systems additional dependencies may be required:
```console
$ apt-get install flex bison libmount-dev
```

Clone the `gst-build` repository, specifying a stable version
```console
$ git clone --depth 1 --branch 1.18.4 https://gitlab.freedesktop.org/gstreamer/gst-build.git gst-build
```

Copy the contents of this repository into `gst-build/subprojects`
```console
$ cp -R /path/to/your/clone/gst-home-audio gst-build/subprojects
```

Then build the project
```console
$ cd gst-build
$ meson builddir -Dcustom_subprojects=gst-home-audio -Dauto_features=disabled -Dgstreamer:tools=enabled
$ ninja -C builddir
```

**Note:** *On macOS, an additional `-Dcpp_std=c++17` flag is needed to build the project*
```console
$ cd gst-build
$ meson builddir -Dcustom_subprojects=gst-home-audio -Dauto_features=disabled -Dgstreamer:tools=enabled -Dcpp_std=c++17
$ ninja -C builddir
```

These commands will build the `gst-home-audio` plugins, along with GStreamer
core, basic plugins, and tools such as `gst-launch-1.0`, `gst-inspect-1.0` etc.

The built plugins can be found in
`gst-build/builddir/subprojects/gst-home-audio/plugins`

To enter development environment run
```console
$ ninja -C builddir devenv
```

## Running
First we have to tell GStreamer where to look for the newly build plugins:

**Note:** *On Windows, install plugins with `ninja -C builddir
install` rather than point them via environment variable.*

```console
$ export GST_PLUGIN_PATH=/path/to/your/clone/gst-home-audio/build/plugins
```

Test if GStreamer can properly retrieve information about the plugins
```console
$ gst-inspect-1.0 dlbac3dec
```

See [Plugins Overview](#plugins-overview) for pipeline examples.

## Integration
The package provides sample integration Python code which can be used for
file-based testing of the included plugins. The integration code is available in
the [`integration`](integration/) directory and includes two frontends:
`gst-ha-flexr` and `gst-ha-dap`.

**Note:** *To run the integration code, make sure that the plugins are installed
in the system, or that `GST_PLUGIN_PATH` is set correctly, as instructed in the
[Running](#running)  section*

### gst-ha-flexr
This integration binary wraps the following plugins:
```console
$ filesrc ! ac3parse ! ac3dec ! dlbflexr ! wavenc ! filesink
```
It takes encoded Dolby Digital/Dolby Digital Plus bitstreams, or WAV PCM files
and outputs decoded and rendered WAV files.

#### How to run it

Navigate to the integration directory.
```console
$ cd integration/
```
To print the full usage, type
```console
$ ./gst-ha-flexr -h
```
#### Configuration files
The `dlbflexr` plugin wrapped with this integration code requires two
configuration files: a stream configuration file, and a device configuration
file.
**Note:** *The configuration files are generated by a propietary tool.*

To decode a bitstream, type:
```console
$ ./gst-ha-flexr -i input.ec3 -s stream.conf -d device.dconf -o output.wav
```
#### Playback
Playback is supported on Windows, Linux and macOS. Note that 48kHz output is required, and macOS supports playback up to eight channels only.

**Note:** *This feature is mutually exclusive with decoding to file.*

To decode and play a bitstream, type:
```console
$ ./gst-ha-flexr -i input.ec3 -s stream.conf -d device.dconf -p
```
First the command invokes the device selection menu to allow a user to choose a target device.
**Note:** *Device selection menu is not available on macOS, audio will be played to default device.*

To decode and play a bitstream directly on a device, type:
```console
$ ./gst-ha-flexr -i input.ec3 -s stream.conf -d device.dconf -p <device id>
```
It will skip device selection menu and start playback immediately. Running `gst-device-monitor-1.0` will show all connected devices with their ids.\
**On macOS, the feature is not supported.**

**Note:** *On Linux, ensure that your device id is correct so that a pipeline does not get stuck*

### gst-ha-dap
The integration binary wraps the following plugins:
```console
filesrc ! dlbaudiodecbin ! dlbdap ! wavenc ! filesink
```
It takes encoded Dolby Digital/Dolby Digital Plus bitstreams and outputs decoded
and rendered WAV files.

#### How to run it
Navigate to the integration directory.
```console
$ cd integration/
```
To print the full usage, type
```console
$ ./gst-ha-dap -h
```
To perform a basic decode to stereo, type
```console
$ ./gst-ha-dap -i input.ec3 -o output.wav
```
#### Configuration files
The integration binary can be provided with two types of configuration files:
`.json` configuration for `dlbdap` plugin, or `.xml` configuration file
generated by Dolby Tuning Tool.  Example of a `.json` configuration file can be
found in [tests directory](tests/files/default-serialized-config.json).

To use the JSON configuration, type
```console
$ ./gst-ha-dap -i input.ec3 -c config.json -o output.wav
```
To use the XML configuration, provide the file name, endpoint name, and profile
name.
```console
$ ./gst-ha-dap -i input.ec3 -x file=config.xml:endpoint=internal-speaker:profile=off -o output.wav
```
#### Dolby Tuning Tool XML conversion
The integration binary can be used to convert XML files generated by the Dolby
tuning tool to JSON format supported by `dlbdap` plugin.
```console
$ ./gst-ha-dap -i input.xml -o output.json
```

## Plugins Overview

### dlbac3parse
Dolby Digital Plus Parser. This plug-in performs parsing of incoming  Dolby
Digital, Dolby Digital Plus signal.

**Launch Line**
```console
$ gst-launch-1.0 filesrc location=<file.ec3> ! dlbac3parse ! dlbac3dec ! \
    wavenc ! filesink location=out.wav
```

### dlbac3dec
Dolby Digital Plus Decoder plug-in enables decoding Dolby Digital, Dolby
Digital Plus bitstreams with and without Dolby Atmos content.

**Launch Line**
```console
$ gst-launch-1.0 filesrc location=<file.ec3> ! dlbac3parse ! dlbac3dec ! \
    wavenc ! filesink location=out.wav
```

### dlbflexr
Dolby Flexible Renderer plug-in enables flexible rendering. Accepts object-
and channel-based audio at input. `device-config` and `stream-config` properties
have to be set and their value must point to serialized config files.

**Launch Line**

Single stream:
```console
$ gst-launch-1.0 \
    audiotestsrc ! \
    dlbflexr device-config=device.conf sink_0::stream-config=stream.conf sink_0::upmix=true ! \
    autoaudiosink
```

Multiple streams:
```console
$ gst-launch-1.0 -v \
    audiotestsrc freq=100 ! \
    dlbflexr name=flexr device-config=device.conf sink_0::stream-config=stream.conf sink_1::stream-config=stream.conf ! autoaudiosink \
    audiotestsrc freq=500 ! flexr.
```

AC3/EAC3 stream:
```console
$ gst-launch-1.0 -v \
    filesrc location=atmos.ec3 ! \
    dlbac3parse ! \
    dlbac3dec ! \
    dlbflexr name=flexr device-config=device.conf sink_0::stream-config=stream.conf ! \
    autoaudiosink
```

### dlboar
Object Audio Renderer plug-in performs rendering of Dolby Atmos content, which
is described by its associated object audio metadata. It should be used after
the decoder for rendering  Dolby Atmos content. Most of the time
`dlbaudiodecbin` should be used instead as this will add `dlboar` only if
needed. `caps` after an element determine rendering configuration.

**Launch Line**
```console
$ gst-launch-1.0 filesrc location=<file.ec3> ! dlbac3parse ! dlbac3dec ! dlboar ! \
    capsfilter caps="audio/x-raw,channels=8,channel-mask=(bitmask)0xc003f" ! \
    wavenc ! filesink location=out.wav
```

### dlbdap
Dolby Audio Processing plug-in processes audio signals using cognitive and
psychoacoustic models of audio perception to consistently enhance the listening
experience for all audio content.

**Launch Line**
```console
$ gst-launch-1.0 filesrc audiotestsrc ! dlbdap json-config=conf.json ! \
    capsfilter caps="audio/x-raw,channels=8,channel-mask=(bitmask)0xc003f" ! \
    wavenc ! filesink location=out.wav
```

The Dolby Audio Processing channel mask uses [gstreamer channel layout](https://gstreamer.freedesktop.org/documentation/audio/gstaudiochannels.html?gi-language=c#GstAudioChannelPosition),
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
in `dap_test_negotiation_tuple`.

### dlbaudiodecbin
The plugin wraps the parser, decoder and object audio renderer into a single element.

**Launch Line**
```console
$ gst-launch-1.0 filesrc location=<file.ec3> ! dlbaudiodecbin ! dlbdap ! \
    capsfilter caps="audio/x-raw,channels=8,channel-mask=(bitmask)0xc003f" ! \
    wavenc ! filesink location=out.wav
```
