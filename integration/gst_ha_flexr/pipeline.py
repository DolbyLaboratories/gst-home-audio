import gi
gi.require_version('Gst', '1.0')
gi.require_version('GstAudio', '1.0')
from gi.repository import GObject, Gst, GstAudio

Gst.init(None)

DECODER_MODE_RAW = 21
DECODER_MODE_CORE = 22

class GstHAFlexrPipeline:
    """Build and run basic file-based pipeline using gst-home-audio plugins."""
    def __init__(self,
                 plugin_path,
                 settings):

        """
        Initializes the pipeline with all necessary elements:
            - filesrc - the file source element
            - dlbac3dec
            - dlbac3parse
            - flexr
            - wavenc - the WAVE encoder element
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
        self.bus.connect('message::eos', self.on_eos)
        self.bus.connect('message::error', self.on_error)

        # Check if plugins already loaded
        registry = Gst.Registry.get()
        plugins = []
        for plugin in registry.get_plugin_list():
            plugins.append(plugin.get_name())
        if not all(x in plugins for x in ['dlbac3parse', 'dlbac3dec', 'dlbflexr']):
            # Add gst-home-audio plugins from specified path if not loaded before
            assert registry.scan_path(plugin_path)

        # File source
        self.src = Gst.ElementFactory.make('filesrc', 'file-src')
        self.src.set_property('location', settings.ifile)

        if settings.input_type == "bitstream":
            # AC3 parser
            self.dec1 = Gst.ElementFactory.make('dlbac3parse', 'ac3-parser')
            # AC3 decoder
            self.dec2 = Gst.ElementFactory.make('dlbac3dec', 'ac3-dec')
        else:
            # Decode bin to parse WAV
            self.dec1 = Gst.ElementFactory.make('decodebin', 'dec-bin')
            self.dec1.connect("pad-added", self._on_pad_added)
            # Audio convert
            self.dec2 = Gst.ElementFactory.make('audioconvert', 'convert')

        # Flexible renderer
        self.flexr = Gst.ElementFactory.make('dlbflexr', 'flexr')
        # Set element properties
        self.flexr.set_property('device-config', settings.dconf)
        if settings.active_channels_enable:
            self.flexr.set_property('active-channels-enable', True)
            self.flexr.set_property('active-channels-mask', settings.active_channels_bitmask)
        # Add a single sink pad
        self.flexr_sink_pad = self.flexr.request_pad(self.flexr.get_pad_template('sink_%u'),
                                                     'sink_0',
                                                     None)
        # Set pad properies
        self.flexr_sink_pad.set_property('stream-config', settings.sconf)
        self.flexr_sink_pad.set_property('upmix', settings.upmix)

        # WAV encoder
        self.wavenc = Gst.ElementFactory.make('wavenc', 'wav-enc')

        # File sink
        self.sink = Gst.ElementFactory.make('filesink', 'file-sink')
        self.sink.set_property('location', settings.ofile)

        # Populate the pipeline
        self.pipeline.add(self.src)
        self.pipeline.add(self.dec1)
        self.pipeline.add(self.dec2)
        self.pipeline.add(self.flexr)
        self.pipeline.add(self.wavenc)
        self.pipeline.add(self.sink)

        # Link elements
        self.src.link(self.dec1)
        if settings.input_type == "bitstream":
            # If bitstream is decoded, we can connect right away.
            # Otherwise, we have to wait for a pad.
            self.dec1.link(self.dec2)
        self.dec2.link(self.flexr)
        self.flexr.link(self.wavenc)
        self.wavenc.link(self.sink)

    def run(self):
        """Used to run the pipeline."""
        self.pipeline.set_state(Gst.State.PLAYING)
        self.mainloop.run()

    def kill(self):
        """Used to stop the pipeline."""
        self.pipeline.set_state(Gst.State.NULL)
        self.mainloop.quit()

    def _on_pad_added(self, decodebin, pad):
        """Used as callback to connect decodebin to the pipeline."""
        caps = pad.get_current_caps()
        compatible_pad = self.dec2.get_compatible_pad(pad, caps)
        pad.link(compatible_pad)

    def on_eos(self, bus, msg):
        """Used as callback to stop pipeline if EOS message is received."""
        print('End of stream (EOS) received. Stopping pipeline.')
        self.kill()

    def on_error(self, bus, msg):
        """Used as callback to stop pipeline if error message is received."""
        print('Error: {}'.format(msg.parse_error()))
        self.kill()
