import os

class gst_ha_flexr_settings:
    """Wrapper for settings used by ::class::`GstHomeAudioFlexrPipeline`."""
    def __init__(self):
        self.ifile = ""
        self.ofile = ""
        self.sconf = ""
        self.dconf = ""
        self.upmix = False
        self.active_channels_enable = False
        self.active_channels_bitmask = 0
        self.input_type = ""

    def _create_active_channels_bitmask(self, ch_list):
        """Creates a gstreamer channel mask from provided list of active
           outputs.
        
        Args:
            ch_list: A comma-separated list of channel indices
        """
        for i in ch_list.split(','):
            self.active_channels_bitmask = self.active_channels_bitmask | 1 << int(i)

    def create_from_args(self, args):
        """Creates settings from supplied command line arguments.

        Args:
            args: A ::class::`argparse.Namespace` instance with command line
                arguments and their values.

        """
        # Write file names
        self.ifile = os.path.abspath(args.input)
        self.ofile = os.path.abspath(args.output)
        self.sconf = os.path.abspath(args.sconf)
        self.dconf = os.path.abspath(args.dconf)

        # Write active channel bitmask
        if args.active_channels is not None:
            self.active_channels_enable = True
            self._create_active_channels_bitmask(args.active_channels)
        
        # Write other settings
        self.upmix = args.upmix

        _, ext = os.path.splitext(args.input)
        if ext == ".wav":
            self.input_type = "pcm"
        else:
            self.input_type = "bitstream"
