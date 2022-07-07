import gi

gi.require_version("Gst", "1.0")
gi.require_version("GstAudio", "1.0")
from gi.repository import GObject, Gst, GstAudio
import platform

Gst.init(None)

DECODER_MODE_RAW = 21
DECODER_MODE_CORE = 22

DD_TYPES = [
    "audio/x-ac3",
    "audio/x-eac3",
    "audio/ac3",
    "audio/eac3",
    "audio/x-private1-ac3",
]


class GstHAFlexrPipeline:
    """Build and run basic file-based pipeline using gst-home-audio plugins."""

    def __init__(self, plugin_path, settings):
        """
        Initializes the pipeline with all necessary elements:
            - filesrc - the file source element
            - flexr
            - filesink - the file sink element

        Args:
            plugin_path: Absolute path to the directory with built Gstreamer
                plugins.
            settings: A ::class::`gst_ha_flexr_settings` instance with
                setting for the pipeline.
        """

        self.mainloop = GObject.MainLoop()
        self.pipeline = Gst.Pipeline()
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect("message::eos", self.on_eos)
        self.bus.connect("message::error", self.on_error)
        self.input_type = ""
        self.settings = settings

        # Check if plugins already loaded
        registry = Gst.Registry.get()
        plugins = []
        for plugin in registry.get_plugin_list():
            plugins.append(plugin.get_name())
        if not all(x in plugins for x in ["dlbac3parse", "dlbac3dec", "dlbflexr"]):
            # Add gst-home-audio plugins from specified path if not loaded before
            assert registry.scan_path(plugin_path)

        # File source
        self.src = Gst.ElementFactory.make("filesrc", "file-src")
        self.src.set_property("location", self.settings.ifile)

        # Typefind
        self.typefind = Gst.ElementFactory.make("typefind")
        self.typefind.connect("have-type", self._have_type)

        # Flexible renderer
        self.flexr = Gst.ElementFactory.make("dlbflexr", "flexr")
        # Set element properties
        self.flexr.set_property("device-config", self.settings.dconf)
        self.flexr.set_property("external-user-gain", self.settings.external_gain)
        self.flexr.set_property("external-user-gain-by-step",
                                self.settings.external_gain_by_step)
        if self.settings.active_channels_enable:
            self.flexr.set_property("active-channels-enable", True)
            self.flexr.set_property(
                "active-channels-mask", self.settings.active_channels_bitmask
            )
        # Add a single sink pad
        self.flexr_sink_pad = self.flexr.request_pad(
            self.flexr.get_pad_template("sink_%u"), "sink_0", None
        )
        # Set pad properies
        self.flexr_sink_pad.set_property("stream-config", self.settings.sconf)
        self.flexr_sink_pad.set_property("upmix", self.settings.upmix)
        self.flexr_sink_pad.set_property("content-normalization-gain",
                                         self.settings.content_gain)
        self.flexr_sink_pad.set_property("internal-user-gain", self.settings.internal_gain)
        self.flexr_sink_pad.set_property("interp-mode", self.settings.interp_mode)

        # Populate the pipeline
        self.pipeline.add(self.src)
        self.pipeline.add(self.typefind)
        self.pipeline.add(self.flexr)

        # Link elements
        self.src.link(self.typefind)
        self._link_sink()

    def run(self):
        """Used to run the pipeline."""
        self.pipeline.set_state(Gst.State.PLAYING)
        self.mainloop.run()

    def kill(self):
        """Used to stop the pipeline."""
        self.pipeline.set_state(Gst.State.NULL)
        self.mainloop.quit()

    def error_kill(self):
        """Used to stop pipeline if error occurs"""
        self.pipeline.set_state(Gst.State.PAUSED)
        self.mainloop.quit()

    def _on_pad_added(self, decodebin, pad):
        """Used as callback to connect decodebin to the pipeline."""
        caps = pad.get_current_caps()
        compatible_pad = self.dec2.get_compatible_pad(pad, caps)
        pad.link(compatible_pad)

    def _demux_pad_added(self, demux, pad):
        """Used as callback to connect qtdemux to the pipeline."""
        if pad.name == "audio_0":
            pad.link(self.dec1.get_static_pad("sink"))

    def _have_type(self, typefind, probability, caps):
        """Used as callback to create and link pipeline on media type detection"""

        self.input_type = caps.to_string()

        # Set flexr pad properties dependent on file type
        force_order = False
        set_profile = self.settings.profile != "off"
        
        if self.input_type in DD_TYPES or "video/quicktime" in self.input_type:
            force_order = True
            if set_profile:
                profile_name = "dlb-"+self.settings.profile
        else:
            if set_profile:
                profile_name = self.settings.profile

        self.flexr_sink_pad.set_property("force-order", force_order)
        
        if set_profile:
            self.flexr_sink_pad.set_property(
                "content-normalization-gain",
                self.settings.content_gain_for_profile[profile_name]
            )

        # Add elements to pipeline according to input type
        if "video/quicktime" in self.input_type:
            # Demuxer for MP4 input
            self.demux = Gst.ElementFactory.make("qtdemux", "demux")
            self.demux.connect("pad-added", self._demux_pad_added)
            self.pipeline.add(self.demux)
            # AC3 parser
            self.dec1 = Gst.ElementFactory.make("dlbac3parse", "ac3-parser")
            # AC3 decoder
            self.dec2 = Gst.ElementFactory.make("dlbac3dec", "ac3-dec")

        elif self.input_type == "audio/x-wav":
            # Decode bin to parse WAV
            self.dec1 = Gst.ElementFactory.make("decodebin", "dec-bin")
            self.dec1.connect("pad-added", self._on_pad_added)
            # Audio convert
            self.dec2 = Gst.ElementFactory.make("audioconvert", "convert")
            # Resampler
            self.resampler = Gst.ElementFactory.make("audioresample")
            self.pipeline.add(self.resampler)

        elif self.input_type in DD_TYPES:
            # AC3 parser
            self.dec1 = Gst.ElementFactory.make("dlbac3parse", "ac3-parser")
            # AC3 decoder
            self.dec2 = Gst.ElementFactory.make("dlbac3dec", "ac3-dec")

        else:
            print("Error: Unable to create pipeline - unsupported input file format.")
            self.error_kill()
            return

        self.pipeline.add(self.dec1)
        self.pipeline.add(self.dec2)

        # Link elements
        if "video/quicktime" in self.input_type:
            self.typefind.link(self.demux)
            self.dec1.link(self.dec2)
            self.dec2.link(self.flexr)

        elif "audio/x-wav" in self.input_type:
            self.typefind.link(self.dec1)
            self.dec2.link(self.resampler)
            self.resampler.link(self.flexr)

        elif self.input_type in DD_TYPES:
            # If bitstream is decoded, we can connect right away.
            # Otherwise, we have to wait for a pad.
            self.typefind.link(self.dec1)
            self.dec1.link(self.dec2)
            self.dec2.link(self.flexr)

        # Sync with pipeline state starting with downstream elements
        self.dec2.sync_state_with_parent()
        self.dec1.sync_state_with_parent()
        if "video/quicktime" in self.input_type:
            self.demux.sync_state_with_parent()
        if "audio/x-wav" in self.input_type:
            self.resampler.sync_state_with_parent()

    def _windows_audiosink(self, device):
        """Returns audio sink specific for Windows"""

        # Audioconvert
        audioconvert = Gst.ElementFactory.make("audioconvert")
        # Add element to the pipeline
        self.pipeline.add(audioconvert)

        # link audioconvert to flexr
        self.flexr.link(audioconvert)

        # Windows audio sink
        wasapisink = Gst.ElementFactory.make("wasapisink")
        wasapisink.set_property("device", device)

        # Add and link element
        self.pipeline.add(wasapisink)
        audioconvert.link(wasapisink)

    def _linux_audiosink(self, device):
        """Returns audio sink specific for Linux"""

        # Audioconvert
        audioconvert = Gst.ElementFactory.make("audioconvert")
        # Add element to the pipeline
        self.pipeline.add(audioconvert)

        # Link audioconvert to tee
        self.flexr.link(audioconvert)

        # Create pulsesink object and set chosen device
        pulsesink = Gst.ElementFactory.make("pulsesink")

        pulsesink.set_property("device", device)

        # Add and link element
        self.pipeline.add(pulsesink)
        audioconvert.link(pulsesink)

    def _darwin_audiosink(self, device):
        """Returns audio sink specific for OSX"""
        print("WIP OSX Playback")

        # Audioconvert
        audioconvert = Gst.ElementFactory.make("audioconvert")
        # Add element to the pipeline
        self.pipeline.add(audioconvert)

        # Link audioconvert to tee
        self.flexr.link(audioconvert)

        # Create autoaudiosink element
        autoaudiosink = Gst.ElementFactory.make("autoaudiosink")
        # Add and link element
        self.pipeline.add(autoaudiosink)
        audioconvert.link(autoaudiosink)

    def _unsupported_platform(self):
        """Exit program if playback is not supported on that platform"""
        print(
            f"Error: Playback is not supported on this platform ({platform.system()})"
        )
        self.kill()

    def _filesink_output(self):
        """Creates and links filesink to the pipeline"""

        # WAV encoder
        self.wavenc = Gst.ElementFactory.make("wavenc", "wav-enc")

        # File sink
        self.sink = Gst.ElementFactory.make("filesink", "file-sink")
        self.sink.set_property("location", self.settings.ofile)
        self.pipeline.add(self.wavenc)
        self.pipeline.add(self.sink)

        # Link elements
        self.flexr.link(self.wavenc)
        self.wavenc.link(self.sink)

    def _audiosink_output(self):
        """If playback option was chosen run system specific pipeline"""

        os = platform.system()

        system = {
            "Windows": self._windows_audiosink,
            "Linux": self._linux_audiosink,
            "Darwin": self._darwin_audiosink,
        }

        sink = system.get(os, self._unsupported_platform)
        sink(self.settings.playback_device)

    def _link_sink(self):
        """Links correct sink to the pipeline.

        Parameters
        ----------
        settings :
            gst_ha_flexr_settings class object
        """

        if self.settings.ofile:
            self._filesink_output()
        elif self.settings.playback_device:
            self._audiosink_output()

    def on_eos(self, bus, msg):
        """Used as callback to stop pipeline if EOS message is received."""
        print("End of stream (EOS) received. Stopping pipeline.")
        self.kill()

    def on_error(self, bus, msg):
        """Used as callback to stop pipeline if error message is received."""
        print("Error: {}".format(msg.parse_error()))
        self.kill()

    def debug_pipeline_to_file(self, path):
        """Saves pipeline graph in .dot file."""

        with open(path, "w") as f:
            f.write(Gst.debug_bin_to_dot_data(self.pipeline, Gst.DebugGraphDetails.ALL))
