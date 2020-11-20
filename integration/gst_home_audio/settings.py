#!/usr/bin/env python

import os
import gi
gi.require_version('Gst', '1.0')
gi.require_version('GstAudio', '1.0')
from gi.repository import GstAudio

AUDIO_CHANNEL_POSITIONS = {
    "lr": [
        GstAudio.AudioChannelPosition.FRONT_LEFT,
        GstAudio.AudioChannelPosition.FRONT_RIGHT
    ],
    "c": [
        GstAudio.AudioChannelPosition.FRONT_CENTER
    ],
    "lfe": [
        GstAudio.AudioChannelPosition.LFE1
    ],
    "lrs": [
        GstAudio.AudioChannelPosition.REAR_LEFT,
        GstAudio.AudioChannelPosition.REAR_RIGHT
    ],
    "lrrs": [
        GstAudio.AudioChannelPosition.SIDE_LEFT,
        GstAudio.AudioChannelPosition.SIDE_RIGHT
    ],
    "lrtf": [
        GstAudio.AudioChannelPosition.TOP_FRONT_LEFT,
        GstAudio.AudioChannelPosition.TOP_FRONT_RIGHT
    ],
    "lrtm": [
        GstAudio.AudioChannelPosition.TOP_SIDE_LEFT,
        GstAudio.AudioChannelPosition.TOP_SIDE_RIGHT
    ],
    "lrtr": [
        GstAudio.AudioChannelPosition.TOP_REAR_LEFT,
        GstAudio.AudioChannelPosition.TOP_REAR_RIGHT
    ],
    "sh": [
        GstAudio.AudioChannelPosition.TOP_CENTER
    ]
}


class dec_drc_settings:
    """Wrapper for dlbaudiodecbin element settings.

    Args:
       mode: Sets the decoder dynamic range control mode:
           (0): disable
           (1): enable
           (2): auto
       cut: Sets the dynamic range control cut scale factor
       boost: Sets the dynamic range control boost scale factor

    """
    def __init__(self,
                 mode=1,
                 cut=1,
                 boost=1):

        self.mode = mode
        self.cut = cut
        self.boost = boost


class dap_global_settings:
    """Wrapper for dlbdap global setting structure.

    Args:
        use_serialized_settings: If true, the serialized configuration from
            the JSON configuration file will be used.
        virtualizer_enable: If true, speaker virtualizer will be enabled.
        override_virtualizer_settings: If true, the speaker virtualizer
            settings from the serialized configuration will be overriden by
            speaker virtualizer settings in the ``virtualizer-settings``
            object in the JSON configuration file.

    """
    def __init__(self,
                 use_serialized_settings=False,
                 virtualizer_enable=False,
                 override_virtualizer_settings=False):

        self.use_serialized_settings = use_serialized_settings
        self.virtualizer_enable = virtualizer_enable
        self.override_virtualizer_settings = override_virtualizer_settings


class gst_home_audio_settings:
    """Wrapper for settings used by ::class::`GstHomeAudioPipeline`."""
    def __init__(self):
        self.ifile = ""
        self.ofile = ""
        self.speaker_list = []
        self.n_lfe = 0
        self.n_height = 0
        self.n_channels = 0
        self.speaker_bitmask = ""
        self.drc_settings = dec_drc_settings()
        self.global_settings = dap_global_settings()

    def create_from_args(self, args):
        """Creates settings from supplied command line arguments.

        Args:
            args: A ::class::`argparse.Namespace` instance with command line
                arguments and their values.

        """
        # Write file names
        self.ifile = os.path.abspath(args.input)
        self.ofile = os.path.abspath(args.output)

        # Validate and set speaker configuration
        if bool(args.speakers):
            (self.speaker_list,
             self.n_lfe,
             self.n_height) = self.validate_speaker_config(args.speakers)

            (self.n_channels,
             self.speaker_bitmask) = self.convert_speakers_to_bitmask(self.speaker_list)
        else:
            self.n_channels = 0
            self.n_height = 0
            self.n_lfe = 0
            self.speaker_bitmask = ""

        # Write drc settings
        self.drc_settings = dec_drc_settings()
        if args.drc is not None:
            for sets in args.drc.split(':'):
                option = sets.split('=')
                if option[0] == 'mode':
                    if option[1] == 'disable':
                        self.drc_settings.mode = 0
                    elif option[1] == 'enable':
                        self.drc_settings.mode = 1
                    elif option[1] == 'auto':
                        self.drc_settings.mode = 2
                elif option[0] == 'x':
                    self.drc_settings.cut = float(option[1])
                elif option[0] == 'y':
                    self.drc_settings.boost = float(option[1])

    @staticmethod
    def validate_speaker_config(speakers):
        """ Validates the correctness of provided speaker layout and remaps Dolby Atmos
        Enabled speakers.

        Args:
            speakers: A colon-separated list of speakers, e.g.
                ``lr:c:lfe:lrs:lre``

        Returns:
            speaker_list: A list of validated speaker pair names.
            lfe_present: 1 if LFE speaker is present in the layout, else 0.
            num_height_speakers: Number of height speakers in the layout.

        """
        num_height_speakers = 0
        num_daes = 0
        lfe_present = 0

        speaker_list = speakers.split(':')

        # Check speaker prerequisites
        for spk in speaker_list:
            if spk == "lfe":
                lfe_present = 1
            # speakers that require lr
            if spk in ["c", "lfe", "lrs", "lre", "lrtm", "lrtf", "sh"]:
                assert "lr" in speaker_list, (
                    "Invalid Dolby Atmos speaker layout!\n"
                    "Speaker prerequisites not met.\n"
                    "Speakers \'%s\' require \'lr\'." % (spk))
            # speakers that require lrs
            if spk in ["lrrs", "lrse", "lrtr"]:
                assert "lrs" in speaker_list, (
                    "Invalid Dolby Atmos speaker layout!\n"
                    "Speaker prerequisites not met.\n"
                    "Speakers \'%s\' require \'lrs\'." % (spk))
            # speakers that require lrrs
            if spk == "lrrse":
                assert "lrrs" in speaker_list, (
                    "Invalid Dolby Atmos speaker layout!\n"
                    "Speaker prerequisites not met.\n"
                    "Speakers \'%s\' require \'lrrs\'." % (spk))
            # validate single height
            if spk == "sh":
                assert (not any(x in speaker_list for x in ["lrrs", "lre", "lrse", "lrrse", "lrtf", "lrtm", "lrtr"])), (
                    "Invalid Dolby Atmos speaker layout! "
                    "The Single height speaker can be used in speaker layouts:"
                    "2.x.1, 3.x.1, 4.x.1, or 5.x.1.")
                num_height_speakers = 1
            # count height speakers
            if spk in ["lre", "lrse", "lrrse", "lrtf", "lrtm", "lrtr"]:
                num_height_speakers = num_height_speakers + 2
            # count DAES
            if spk in ["lre", "lrse", "lrrse"]:
                num_daes = num_daes + 2

        # Check if maximum number of heights is not exceeded
        assert num_height_speakers <= 4, (
            "Invalid Dolby Atmos speaker layout!\n"
            "The maximum supported number of height speakers is 4.")

        # remap DAES
        if num_daes == 2:
            assert "lrtm" not in speaker_list, (
                "Invalid Dolby Atmos speaker layout!\n"
                "Unable to remap Dolby Atmos Enabled speakers!")
            speaker_list.append("lrtm")
        if num_daes == 4:
            assert "lrtf" not in speaker_list, (
                "Invalid Dolby Atmos speaker layout!\n"
                "Unable to remap Dolby Atmos Enabled speakers!")
            assert "lrtr" not in speaker_list, (
                "Invalid Dolby Atmos speaker layout!\n"
                "Unable to remap Dolby Atmos Enabled speakers!")
            speaker_list.append("lrtf")
            speaker_list.append("lrtr")

        for spk in ["lre", "lrse", "lrrse"]:
            if spk in speaker_list:
                speaker_list.remove(spk)

        # Check for invalid height speaker combinations
        if num_height_speakers == 4:
            assert not (all(x in speaker_list for x in ['lrtf', 'lrtm']) or
                        all(x in speaker_list for x in ['lrtm', 'lrtr'])), (
                            "Invalid Dolby Atmos speaker layout!\n"
                            "For two pairs of heights only LRtf and LRtr "
                            "are allowed.")

        # If there is only one pair of height speakers, remap them to lrtm
        if num_height_speakers == 2:
            for spk in speaker_list:
                if spk in ['lrtf', 'lrtr']:
                    idx = speaker_list.index(spk)
                    speaker_list[idx] = "lrtm"

        return speaker_list, lfe_present, num_height_speakers

    @staticmethod
    def convert_speakers_to_bitmask(speaker_list):
        """Converts a list of speaker names into a Gstreamer bitmask.

        Args:
            speaker_list: A list of speaker pair names, e.g.
                ``[lr, c, lfe, lrs, lrtm]``

        Returns:
            num_channels: Number of audio channels in provided configuration.
            bitmask: A Gstreamer bitmask for the specified configuration.

        """
        chs = []
        num_channels = 0
        for item in speaker_list:
            positions = AUDIO_CHANNEL_POSITIONS[item]
            chs.extend(positions)
            num_channels = num_channels + len(positions)
            _, bitmask = GstAudio.audio_channel_positions_to_mask(chs, False)
        return num_channels, bitmask
