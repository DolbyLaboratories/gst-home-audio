#!/usr/bin/env python

import unittest
import os
import sys
import gi

gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst
from pathlib import Path

testfiles_path = Path(os.environ['GST_TEST_FILES_PATH'])

class TestSanity(unittest.TestCase):
    def setUp(self):
        Gst.init(None)

    def test_pipeline(self):
        test_file = testfiles_path.joinpath('51_1kHz_ddp.ec3')
        pipeline = Gst.parse_launch(
            "filesrc location={} ! dlbac3parse ! dlbac3dec ! fakesink".format(test_file))

        pipeline.set_state(Gst.State.PLAYING)

if __name__ == '__main__':
    unittest.main()
