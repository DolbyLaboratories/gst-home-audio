#!/usr/bin/env python

import unittest
import os
import sys
import gi

gi.require_version("Gst", "1.0")
from gi.repository import GObject, Gst
from pathlib import Path

testfiles_path = Path(os.environ["GST_TEST_FILES_PATH"])


class TestSanity(unittest.TestCase):
    def test_pipeline(self):
        test_file = testfiles_path.joinpath("51_1kHz_ddp.ec3")
        pipeline = Gst.parse_launch(
            "filesrc location={} ! dlbac3parse ! dlbac3dec ! fakesink".format(test_file)
        )

        pipeline.set_state(Gst.State.PLAYING)


if __name__ == "__main__":
    Gst.init(None)

    registry = Gst.Registry.get()
    plugins = list(map(lambda p: p.get_name(), registry.get_plugin_list()))

    if not all(p in plugins for p in ["dlbac3parse", "dlbac3dec"]):
        # skip test
        sys.exit(77)

    unittest.main()
