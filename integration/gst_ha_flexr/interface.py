from typing import Tuple
import gi

gi.require_version("Gst", "1.0")
gi.require_version("GstAudio", "1.0")
from gi.repository import Gst

import os
import argparse
from .settings import gst_ha_flexr_settings
from .pipeline import GstHAFlexrPipeline


VERSION_MAJOR = 0
VERSION_MINOR = 9
VERSION_BUGFIX = 9

PREAMBLE = """
Dolby Atmos for home audio executable
Version %d.%d.%d

Supported formats:
    Dolby Digital:      .ac3
    Dolby Digital Plus: .ec3
    PCM:                .wav
    MP4:                .mp4

""" % (
    VERSION_MAJOR,
    VERSION_MINOR,
    VERSION_BUGFIX,
)

def parse_command_line(my_args=None) -> argparse.Namespace:
    """
    Command line interface parser.

    Returns:
        args: A ::class::`argparse.Namespace` instance holding command line
            arguments and their values.
    """
    parser = argparse.ArgumentParser(
        prog="gst-ha-flexr", formatter_class=argparse.RawTextHelpFormatter
    )
    sinks = parser.add_mutually_exclusive_group()
    input_gains = parser.add_mutually_exclusive_group()
    external_gains = parser.add_mutually_exclusive_group()
    parser.add_argument(
        "-wd",
        "--working_dir",
        type=str,
        help=argparse.SUPPRESS,
        required=True,
    )
    parser.add_argument(
        "-i",
        "--input",
        help="Input file name.\n"
        "Provide an input signal to decode.\n"
        "Supported formats:\n"
        "Dolby Digital:      .ac3\n"
        "Dolby Digital Plus: .ec3\n"
        "PCM:                .wav\n"
        "MP4:                .mp4\n\n",
        type=str,
        metavar="<filename>",
        required=True,
    )
    sinks.add_argument(
        "-o",
        "--output",
        help="Defines the output WAV file name.\n"
        "(default value: out.wav)\n"
        "Not allowed with playback (-p argument)\n\n",
        type=str,
        metavar="<filename>",
        nargs="?",
        const="out.wav",
    )
    sinks.add_argument(
        "-p",
        "--playback",
        help="Plays audio on chosen device\n"
        "Device can be chosen from list or defined with ID\n"
        "Not allowed with file output (-o argument)\n\n",
        type=str,
        metavar="<device_ID>",
        nargs="?",
        const=True,
    )
    parser.add_argument(
        "-d",
        "--dconf",
        help="Device configuration file name.\n\n",
        type=str,
        metavar="<filename>",
        required=True,
    )
    parser.add_argument(
        "-s",
        "--sconf",
        help="Stream configuration file name.\n\n",
        type=str,
        metavar="<filename>",
        required=True,
    )
    parser.add_argument(
        "-a",
        "--active-channels",
        help="Comma-separated list of active channel indices.\n\n",
        type=str,
        metavar="<ch_id,ch_id,...>",
        default=None,
    )
    parser.add_argument(
        "-u",
        "--upmix",
        help="Enable upmixing\n\n",
        action="store_true",
        default=False,
    )
    input_gains.add_argument(
        "-pr",
        "--profile",
        help="Select content profile.\n"
        "Default: off\n"
        "Not allowed with content normalization gain (-cg argument).\n\n",
        type=str,
        metavar="<off|movie|music>",
        default="off",
    )
    input_gains.add_argument(
        "-cg",
        "--content-normalization-gain",
        help="Linear gain to bring the input to system level.\n"
        "Range: [0.0 - 10.0]\n"
        "Default: 1.0\n"
        "Not allowed with profile (-pr argument)\n\n",
        type=float,
        metavar="<gain>",
        default=1.0
    )
    parser.add_argument(
        "-ig",
        "--internal-user-gain",
        help="Linear gain as determined by user.\n"
        "Range: [0.0 - 10.0]\n"
        "Default: 1.0\n\n",
        type=float,
        metavar="<gain>",
        default=1.0
    )
    external_gains.add_argument(
        "-eg",
        "--external-user-gain",
        help="Linear gain to be applied by downstream external processing.\n"
        "Range: [0.0 - 10.0]\n"
        "Default: 1.0\n\n",
        type=float,
        metavar="<gain>",
        default=1.0
    )
    external_gains.add_argument(
        "-egs",
        "--external-user-gain-by-step",
        help="Linear gain to be applied by downstream external processing.\n"
        "This property is an alternative form of external-user-gain \n"
        "where instead of using the linear gain, index into the volume \n"
        "steps defined in device configuration is used.\n"
        "Default: disabled - flag not set.\n\n",
        type=int,
        metavar="<vol_step>",
        default=-1
    )
    parser.add_argument(
        "-im",
        "--interp-mode",
        help="Selects interpolation mode.\n"
        "Default: offline\n\n",
        type=str,
        metavar="<offline|runtime>",
        default="offline"
    )
    parser.add_argument(
        "--debug",
        help=argparse.SUPPRESS,
        action="store_true",
        default=False,
    )
    parser.add_argument(
        "-gd",
        "--gst-debug",
        help="Number specyfing Gstreamer Debug Level\n\n",
        type=int,
        metavar="<val>",
        default=0,
    )
    parser.add_argument(
        "-pg",
        "--pipeline-graph",
        help="Defines debug dot file name\n\n",
        type=str,
        metavar="<filename>",
        default=None,
    )

    print(PREAMBLE)

    return parser.parse_args(my_args)


def validate_command_line(args) -> Tuple[bool, str]:
    """
    Validates the correctness of command line arguments.

    Args:
        args: A ::class::`argparse.Namespace` instance with command line
            arguments and their values.

    Returns:
        status: True if the validation was successful, else False.
        message: Error message if the validation failed.
    """
    if not os.path.isfile(args.sconf):
        return False, "Cannot open stream configuration file: %s" % args.sconf

    if not os.path.isfile(args.dconf):
        return False, "Cannot open device configuration file: %s" % args.dconf

    if not os.path.isfile(args.input):
        return False, "Cannot open input file: %s" % args.input

    if args.active_channels is not None:
        for i in args.active_channels.split(","):
            try:
                int(i)
            except ValueError:
                return False, (
                    "Malformed active channels list. Provide the "
                    "active channels as a comma-separated list, e.g. "
                    "0,1,2"
                )

    for arg in [args.content_normalization_gain,
                args.internal_user_gain,
                args.external_user_gain]:
        if arg < 0.0 or arg > 10.0:
            return False, (
                "Invalid gain value. Gain must be in range [0.0 - 10.0]"
            )

    if args.profile is not None:
        if args.profile not in ["off", "music", "movie"]:
            return False, (
                "Invalid profile name. Allowed options are:\n"
                "    off\n"
                "    movie\n"
                "    music"
            )

    if args.interp_mode is not None:
        if args.interp_mode not in ["offline", "runtime"]:
            return False, (
                "Invalid interpolation mode. Allowed options are:\n"
                "    offline\n"
                "    runtime"
            )

    return True, "OK"


def create_settings(args) -> gst_ha_flexr_settings:
    """Runs a validation on command line arguments, and creates a structure
    of settings for the pipeline.

    Args:
        args: A ::class::`argparse.Namespace` instance with command line
            arguments and their values.

    Returns:
        settings: A ::class::`gst_ha_flexr_settings` instance with settings
            for the pipeline.
    """
    ret, msg = validate_command_line(args)
    assert ret, "Invalid Dolby Atmos processing options.\n%s\n" % msg

    settings = gst_ha_flexr_settings()
    settings.create_from_args(args)

    return settings


def gst_debug(level):
    """Sets GStreamer debug output

    Args:
        level : GStreamer debug level.
    """
    if level:
        Gst.debug_set_active(True)
        Gst.debug_set_default_threshold(level)


def run_pipeline(plugin_path, settings):
    """Creates and runs the GStreamer pipeline.

    Args:
        plugin_path: An absolute path to the directory with
            GStreamer plugins.
        settings: A ::class::`gst_home_audio_settings` instance with
            settings for the pipeline.
    """
    pipeline = GstHAFlexrPipeline(plugin_path, settings)
    pipeline.run()
    if settings.pipeline_graph:
        pipeline.debug_pipeline_to_file(settings.pipeline_graph)
