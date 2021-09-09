#!/usr/bin/env python

import gi
gi.require_version('Gst', '1.0')
gi.require_version('GstAudio', '1.0')
from gi.repository import GObject, Gst, GstAudio

Gst.init(None)

DECODER_MODE_RAW = 21
DECODER_MODE_CORE = 22

def convert_timestamp_to_time (timestamp):
    """Convert timestamp to hours, minutes, seconds and useconds."""
    hours = 00
    minutes = 00
    seconds = 00
    useconds =  000000000

    if timestamp != Gst.CLOCK_TIME_NONE:
        hours = timestamp / (Gst.SECOND * 60 * 60)
        minutes = (timestamp / (Gst.SECOND * 60)) % 60
        seconds = (timestamp / Gst.SECOND) % 60
        useconds = timestamp % Gst.SECOND
        
    return hours, minutes, seconds, useconds

class GstHomeAudioPipeline:
    """Build and run basic file-based pipeline using gst-home-audio plugins."""
    def __init__(self,
                 plugin_path,
                 settings,
                 json_config):

        """
        Initializes the pipeline with all necessary elements:
            - filesrc - the file source element
            - dlbaudiodecbin - Gstreamer bin wrapping DD+ parser, decoder
                  and OAR.
            - dlbdap - DAP element
            - capsfilter - used to configure the pipeline into specified
                  channel layout
            - wavenc - the WAVE encoder element
            - filesink - the file sink element

        Args:
            plugin_path: Absolute path to the directory with built Gstreamer
                plugins.
            settings: A ::class::`gst_home_audio_settings` instance with
                setting for the pipeline.
            json_config: Absolute path to JSON confguration file for dlbdap
                element.

        """

        self.mainloop = GObject.MainLoop()
        self.pipeline = Gst.Pipeline()
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message::eos', self.on_eos)
        self.bus.connect('message::error', self.on_error)
        self.bus.connect("message::application", self.on_app_msg)

        # Check if plugins already loaded
        registry = Gst.Registry.get()
        plugins = []
        for plugin in registry.get_plugin_list():
            plugins.append(plugin.get_name())
        if not all(x in plugins for x in ['dlbaudiodecbin', 'dlbdap']):
            # Add gst-home-audio plugins from specified path if not loaded before
            assert registry.scan_path(plugin_path)

        # Check if speaker list was provided
        speaker_list_present = bool(settings.speaker_list)

        if not speaker_list_present:
            # Assume raw mode
            dec_mode = DECODER_MODE_RAW
        else:
            # Set decoder mode
            n_floor = settings.n_channels - settings.n_lfe - settings.n_height
            if ((n_floor < 6) and (settings.n_height == 0) and
                (settings.global_settings.virtualizer_enable is False)):
                # Artistic mixes mode
                dec_mode = DECODER_MODE_CORE
            else:
                # Raw
                dec_mode = DECODER_MODE_RAW

        # File source
        src = Gst.ElementFactory.make('filesrc', 'file-src')
        src.set_property('location', settings.ifile)

        # Audio decoder bin
        dec_bin = Gst.ElementFactory.make('dlbaudiodecbin', 'dec-bin')
        dec_bin.set_property('out-mode', dec_mode)
        dec_bin.set_property('drc-mode', settings.drc_settings.mode)
        dec_bin.set_property('drc-cut', settings.drc_settings.cut)
        dec_bin.set_property('drc-boost', settings.drc_settings.boost)

        # DAP element
        dap = Gst.ElementFactory.make('dlbdap', 'dap')
        dap.set_property('force-order', True)

        # Update decoder mode when serialized configuration is present
        self.dap_bus = Gst.Bus.new()
        dap.set_bus(self.dap_bus)
        if json_config:
            dap.set_property('json-config', json_config)
        msg = self.dap_bus.pop_filtered(Gst.MessageType.ELEMENT)
        if msg:
            self.update_dec_mode(msg, dec_bin)

        # Caps filter
        capsfilter = Gst.ElementFactory.make('capsfilter', 'caps-filter')
        if speaker_list_present:
            caps = 'audio/x-raw,channels=%s,channel-mask=(bitmask)%s' % \
                (settings.n_channels, hex(settings.speaker_bitmask))
            capsfilter.set_property('caps', Gst.Caps.from_string(caps))

        # WAV encoder
        wavenc = Gst.ElementFactory.make('wavenc', 'wav-enc')

        # File sink
        sink = Gst.ElementFactory.make('filesink', 'file-sink')
        sink.set_property('location', settings.ofile)

        # Populate the pipeline
        self.pipeline.add(src)
        self.pipeline.add(dec_bin)
        self.pipeline.add(dap)
        self.pipeline.add(capsfilter)
        self.pipeline.add(wavenc)
        self.pipeline.add(sink)

        # Link elements
        src.link(dec_bin)
        dec_bin.link(dap)
        dap.link(capsfilter)
        capsfilter.link(wavenc)
        wavenc.link(sink)

    @staticmethod
    def update_dec_mode(msg, dec_bin):
        """Verify the processing mode set with the serialized configuration
        and apply correct processing mode. During initialization, the output
        mode for dlbaudiodecbin is set to 21 (raw) if the serialized
        configuration is present. In some cases, when the DAP mode is set to
        2.0 or 5.1, we want to enable core output mode for artistic mixes
        processing. When a serialized configuration is loaded, the DAP plugin
        sends a message with its processing mode, number of channels, and
        virtualization status. This method will modify the default decoding
        mode to 22 (core) if 2.0 or 5.1 decoding mode is detected.

        Args:
            msg: The received message.
            dec_bin: The dlbaudiodecbin element which settings will be changed.
        """
        try:
            struct = msg.get_structure()
        except AttributeError:
            print("Unknown message from element: %s" % msg.src)
            return

        proc_format = struct.get_uint64('processing-format')

        assert proc_format, "Processing format data not found in message."

        bitmask = hex(proc_format.value)

        if bitmask in ['0x3', '0x3f']:
            # If the processing mode is set to 2.0 or 5.1 we need to
            # activate artistic mixes mode. Set the decoder mode to core.
            dec_bin.set_property('out-mode', DECODER_MODE_CORE)

    def run(self):
        """Used to run the pipeline."""
        self.pipeline.set_state(Gst.State.PLAYING)
        self.mainloop.run()

    def kill(self):
        """Used to stop the pipeline."""
        self.pipeline.set_state(Gst.State.NULL)
        self.mainloop.quit()

    def on_eos(self, bus, msg):
        """Used as callback to stop pipeline if EOS message is received."""
        print('End of stream (EOS) received. Stopping pipeline.')
        self.kill()

    def on_error(self, bus, msg):
        """Used as callback to stop pipeline if error message is received."""
        print('Error: {}'.format(msg.parse_error()))
        self.kill()
        
    def on_app_msg(self, bus, msg):
        """Used as callback for app messages."""
        
        tags = msg.get_structure()
        if tags is None:
            return
        
        if tags.get_name() != "stream-info":
            return
        
        hours, minutes, seconds, useconds = convert_timestamp_to_time(msg.timestamp)
        audio_codec = tags.get_string("audio-codec")
        if audio_codec is None:
            return
        
        ret, object_audio = tags.get_boolean("object-audio")
        if not ret: 
            return
        
        ret, surround_decoder_enable = tags.get_boolean("surround-decoder-enable")
        if not ret: 
            return
        
        print("%02u:%02u:%02u:%09u | " %(hours, minutes, seconds, useconds), end='')

        if object_audio:
            print("Dolby Atmos | ", end='')
        elif surround_decoder_enable:
            print("Channel-based with Dolby Surround | ", end='')
        else:
            print("Channel-based | ", end='')
                
        if "E-AC-3" in audio_codec:
            print("Dolby Digital Plus | ")
        elif "AC-3" in audio_codec:
            print("Dolby Digital | ")
        else:
            print("Linear PCM | ")

    def debug_pipeline_to_file(self, path):
        """Saves pipeline graph in .dot file."""

        with open(path, "w") as f:
            f.write(Gst.debug_bin_to_dot_data(self.pipeline, Gst.DebugGraphDetails.ALL))
