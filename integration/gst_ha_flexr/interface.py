import gi
gi.require_version('Gst', '1.0')
gi.require_version('GstAudio', '1.0')
from gi.repository import Gst
import argparse
import os
from .pipeline import GstHAFlexrPipeline
from .settings import gst_ha_flexr_settings

VERSION_MAJOR  = 0
VERSION_MINOR  = 9
VERSION_BUGFIX = 1

PREAMBLE = """
Dolby Atmos for home audio executable
Version %d.%d.%d

Supported formats:
    Dolby Digital:      .ac3
    Dolby Digital Plus: .ec3 
    PCM:                .wav

""" % (VERSION_MAJOR, VERSION_MINOR, VERSION_BUGFIX)

ALLOWED_FILE_EXTENSIONS = [".ac3", ".ec3", ".wav"]

def parse_command_line(my_args=None):
    """
    Command line interface parser.

    Returns:
        args: A ::class::`argparse.Namespace` instance holding command line
            arguments and their values.
    """
    parser = argparse.ArgumentParser(prog='gst-ha-flexr',
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-wd',
                        '--working_dir',
                        type=str,
                        help=argparse.SUPPRESS,
                        required=True)
    parser.add_argument('-i',
                        '--input',
                        help='Input file name.\n'
                             'Provide an input signal to decode.\n'
                             'Supported formats:\n'
                             'Dolby Digital:      .ac3\n'
                             'Dolby Digital Plus: .ec3, .eb3\n'
                             'PCM:                .xml\n\n',
                        type=str,
                        metavar='<filename>',
                        required=True)
    parser.add_argument('-o',
                        '--output',
                        help='Defines the output WAV file name.\n'
                             '(default value: out.wav)\n\n',
                        type=str,
                        metavar='<filename>',
                        default='out.wav')
    parser.add_argument('-d',
                        '--dconf',
                        help='Device configuration file name.\n\n',
                        type=str,
                        metavar='<filename>',
                        required=True)
    parser.add_argument('-s',
                        '--sconf',
                        help='Stream configuration file name.\n\n',
                        type=str,
                        metavar='<filename>',
                        required=True)
    parser.add_argument('-a',
                        '--active-channels',
                        help='Comma-separated list of active channel indices.\n\n',
                        type=str,
                        metavar='<ch_id,ch_id,...>',
                        default=None)
    parser.add_argument('-u',
                        '--upmix',
                        help='Enable upmixing\n\n',
                        action='store_true',
                        default=False)
    parser.add_argument('--debug',
                        help=argparse.SUPPRESS,
                        action='store_true',
                        default=False
                        )
    parser.add_argument('-gd',
                        '--gst_debug',
                        help='Number specyfing Gstreamer Debug Level',
                        type=int,
                        metavar='<val>',
                        default=0)
    parser.add_argument('-pg',
                        '--pipeline_graph',
                        help='Defines debug dot file name \n',
                        type=str,
                        metavar='<filename>',
                        default=None)

    print(PREAMBLE)

    return parser.parse_args(my_args)


def validate_command_line(args):
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
    _, ext = os.path.splitext(args.input)
    if ext not in ALLOWED_FILE_EXTENSIONS:
        return False, "Unsupported input file format: %s" % ext

    if args.active_channels is not None:
        for i in args.active_channels.split(","):
            try:
                int(i)
            except ValueError:
                return False, ("Malformed active channels list. Provide the "
                               "active channels as a comma-separated list, e.g. "
                               "0,1,2")
            
    return True, "OK"


def create_settings(args):
    """Runs a validation on command line arguments, and creates a structure
    of settings for the pipeline.

    Args:
        args: A ::class::`argparse.Namespace` instance with command line
            arguments and their values.

    Returns:
        settings: A ::class::`gst_home_audio_settings` instance with settings
            for the pipeline.
        json_path: An absolute path to the JSON configuration file, or an
            empty string if the configuration was not used.
    """
    ret, msg = validate_command_line(args)
    assert ret, "Invalid Dolby Atmos processing options.\n%s\n" % msg

    settings = gst_ha_flexr_settings()
    settings.create_from_args(args)

    return settings

def gst_debug(level):
    """Sets Gstreamer debug output

    Args:
        level : Gstreamer debug level.
    """ 
    if level:
        Gst.debug_set_active(True)
        Gst.debug_set_default_threshold(level)

def run_pipeline(plugin_path, settings, filename):
    """Creates and runs the Gstreamer pipeline.

    Args:
        plugin_path: An absolute path to the directory with
            Gstreamer plugins.
        settings: A ::class::`gst_home_audio_settings` instance with
            settings for the pipeline.
        json_config: An absolute path to the JSON configuration file,
            or an empty string, if the configuration is not used.
    """
    pipeline = GstHAFlexrPipeline(plugin_path,
                                  settings)
    if filename:
        pipeline.debug_pipeline_to_file(filename)
    pipeline.run()
