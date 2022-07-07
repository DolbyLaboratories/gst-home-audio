import os
import platform
import gi

gi.require_version("Gst", "1.0")
gi.require_version("GstAudio", "1.0")
from gi.repository import Gst


class gst_ha_flexr_settings:
    """Wrapper for settings used by ::class::`GstHomeAudioFlexrPipeline`."""

    def __init__(self):
        self.system = platform.system()
        self.ifile = ""
        self.ofile = ""
        self.sconf = ""
        self.dconf = ""
        self.upmix = False
        self.active_channels_enable = False
        self.active_channels_bitmask = 0
        self.profile = None
        self.content_gain = 1.0
        self.internal_gain = 1.0
        self.external_gain = 1.0
        self.external_gain_by_step = -1
        self.content_gain_for_profile = {
            "off": 1.0,            #  0 dB
            "music": 0.63095,      # -4 dB
            "movie": 1.41253,      # +3 dB
            "dlb-music": 4.46683,  # +13 dB
            "dlb-movie": 3.54813   # +11 dB
        }
        self.interp_mode = None
        self.pipeline_graph = ""
        self.device_obj = None
        self.playback_device = ""
        

    def _create_active_channels_bitmask(self, ch_list):
        """Creates a gstreamer channel mask from provided list of active
           outputs.

        Args:
            ch_list: A comma-separated list of channel indices
        """
        for i in ch_list.split(","):
            self.active_channels_bitmask = self.active_channels_bitmask | 1 << int(i)

    def load_devices(self):
        """Checks for currently available audio sinks

        Returns
        -------
        list
            List of available devices
        """

        device_monitor = Gst.DeviceMonitor.new()
        device_monitor.add_filter("Audio/Sink", None)
        device_monitor.start()
        devices = device_monitor.get_devices()
        device_monitor.stop()

        return devices

    def device_obj_to_str(self):
        """Gets device id from device object"""

        if self.system == "Windows":
            self.playback_device = self.device_obj.get_properties().get_value(
                "device.strid"
            )
        elif self.system == "Linux":
            if not self.device_obj.get_properties().get_value("device.api") == "alsa":
                raise SystemExit("Error: Device API not supported")

            # udev.id uses plus sign which instead of underscore as in device str
            # so udev.id is modified to reflect that
            udev_id = (
                self.device_obj.get_properties().get_value("udev.id").replace("+", "_")
            )
            device_profile = self.device_obj.get_properties().get_value(
                "device.profile.name"
            )
            self.playback_device = f"alsa_output.{udev_id}.{device_profile}"

    def _select_device(self):
        """Allows user to select output device
        by choosing one from list.

        Raises:
        SystemExit
            Exits script if no audio sink devices were found
        """

        if self.system == "Darwin":
            # Device selection is unsupported
            return

        devices = self.load_devices()

        if self.system == "Windows":
            win_devices = []
            for each in devices:
                provider = each.get_properties().get_value("device.api")
                if provider == "wasapi":
                    win_devices.append(each)
            devices = win_devices

        if not devices:
            raise SystemExit("Error: No audio sinks found.")

        print("Available devices: ")
        for i, device in enumerate(devices):
            print(f"[{i+1}] - {device.get_display_name()}")
            try:
                provider = device.get_properties().get_value("device.api")
                print(f"\tProvider: {provider}")
            except AttributeError:
                pass
        device_number = int(input("\nSelect device: "))
        self.device_obj = devices[device_number - 1]
        self.device_obj_to_str()

        print("\nSelected device: ", self.device_obj.get_display_name())
        if self.system == "Windows":
            print(
                "Device strid: ",
                self.device_obj.get_properties().get_value("device.strid"),
                "\n",
            )

    def _device(self, device_id):
        """Saves device id and on Windows it checks if device exists"""

        if self.system == "Windows":
            devices = self.load_devices()

            for device in devices:
                if device.get_properties().get_value("device.strid") == device_id:
                    self.device_obj = device
                    self.playback_device = device_id

            if not self.playback_device:
                raise SystemExit("Error: Device not found")

            print("\nSelected device: ", self.device_obj.get_display_name())
            print(
                "Device strid: ",
                self.device_obj.get_properties().get_value("device.strid"),
                "\n",
            )

        elif self.system == "Linux":
            self.playback_device = device_id

        else:
            raise SystemExit(
                "Error: Playback to specific device is not supported on this platform"
            )

    def create_from_args(self, args):
        """Creates settings from supplied command line arguments.

        Args:
            args: A ::class::`argparse.Namespace` instance with command line
                arguments and their values.

        """
        # Write file names
        self.ifile = os.path.abspath(args.input)
        if args.output:
            self.ofile = os.path.abspath(args.output)
        self.sconf = os.path.abspath(args.sconf)
        self.dconf = os.path.abspath(args.dconf)
        self.pipeline_graph = args.pipeline_graph

        # Write active channel bitmask
        if args.active_channels is not None:
            self.active_channels_enable = True
            self._create_active_channels_bitmask(args.active_channels)

        # Write other settings
        self.upmix = args.upmix
        self.profile = args.profile
        self.content_gain = args.content_normalization_gain
        self.internal_gain = args.internal_user_gain
        self.external_gain = args.external_user_gain
        self.external_gain_by_step = args.external_user_gain_by_step
        self.interp_mode = args.interp_mode
        if type(args.playback) == bool:
            self._select_device()
        elif type(args.playback) == str:
            self._device(args.playback)
