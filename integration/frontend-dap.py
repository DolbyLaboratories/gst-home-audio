#!/usr/bin/env python
import os
import sys
from gst_ha_dap.interface import (parse_command_line,
                                  create_settings,
                                  run_pipeline,
                                  convert_xml)


def main():
    """Main function for the gst-ha-dap command line interface."""
    plugin_path = "../lib/gstreamer-1.0"
    args = parse_command_line()
    if not args.debug:
        sys.tracebacklimit = 0
    if args.input.endswith("xml"):
        convert_xml(args)
    else:
        settings, json_path = create_settings(args)
        run_pipeline(os.path.abspath(plugin_path),
                     settings,
                     json_path)


if __name__ == '__main__':
    main()
