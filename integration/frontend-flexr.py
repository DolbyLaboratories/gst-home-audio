#!/usr/bin/env python
import os
import sys
from gst_ha_flexr.interface import (parse_command_line,
                                    create_settings,
                                    run_pipeline,
                                    gst_debug)


def main():
    """Main function for the gst-ha-dap command line interface."""
    plugin_path = os.path.join(os.path.dirname(__file__), "../lib/gstreamer-1.0")
    args = parse_command_line()
    os.chdir(args.working_dir)

    if not args.debug:
        sys.tracebacklimit = 0
    gst_debug(args.gst_debug)

    settings = create_settings(args)
    run_pipeline(os.path.abspath(plugin_path),
                 settings)


if __name__ == '__main__':
    main()
