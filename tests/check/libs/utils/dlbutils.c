/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2020, Dolby Laboratories

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>

#include "dlbutils.h"

GST_START_TEST (test_dlb_utils_buffer_data_type)
{
  GstAudioInfo info;
  dlb_buffer *buf;

  gst_audio_info_init (&info);

  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S32, 0, 2, NULL);
  buf = dlb_buffer_new (&info);
  fail_unless (buf->data_type == DLB_BUFFER_INT_LEFT);
  dlb_buffer_free (buf);

  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16, 0, 6, NULL);
  buf = dlb_buffer_new (&info);
  fail_unless (buf->data_type == DLB_BUFFER_SHORT_16);
  dlb_buffer_free (buf);

  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_F32, 0, 8, NULL);
  buf = dlb_buffer_new (&info);
  fail_unless (buf->data_type == DLB_BUFFER_FLOAT);
  dlb_buffer_free (buf);

  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_F64, 0, 10, NULL);
  buf = dlb_buffer_new (&info);
  fail_unless (buf->data_type == DLB_BUFFER_DOUBLE);
  dlb_buffer_free (buf);

  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S8, 0, 11, NULL);
  buf = dlb_buffer_new (&info);
  fail_unless (buf->data_type == DLB_BUFFER_OCTET_UNPACKED);
  dlb_buffer_free (buf);

  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_U20, 0, 12, NULL);
  buf = dlb_buffer_new (&info);
  fail_unless (buf == NULL);
}
GST_END_TEST

GST_START_TEST (test_dlb_utils_buffer_reordering)
{
  GstAudioInfo info;
  dlb_buffer *buf;

  gfloat mem[14];
  gint map[14];
  gint i;

  GstAudioChannelPosition gst_pos[] = {
    GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
    GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
    GST_AUDIO_CHANNEL_POSITION_LFE1,
    GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
    GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
    GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT,
    GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT,
    GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_LEFT,
    GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_RIGHT,
  };

  GstAudioChannelPosition dlb_pos[] = {
    GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
    GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
    GST_AUDIO_CHANNEL_POSITION_LFE1,
    GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
    GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
    GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT,
    GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_LEFT,
    GST_AUDIO_CHANNEL_POSITION_TOP_SIDE_RIGHT,
    GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT,
    GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT,
  };

  gst_audio_info_init (&info);
  gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_F32, 48000, 14, gst_pos);
  gst_audio_get_channel_reorder_map (14, gst_pos, dlb_pos, map);

  buf = dlb_buffer_new_wrapped ((guint8 *)mem, &info, TRUE);
  for (i = 0; i < 14; ++i)
    fail_unless_equals_pointer (buf->ppdata[i], &mem[map[i]]);

  dlb_buffer_free (buf);

  buf = dlb_buffer_new_wrapped ((guint8 *)mem, &info, FALSE);
  for (i = 0; i < 14; ++i)
    fail_unless_equals_pointer (buf->ppdata[i], &mem[i]);

  dlb_buffer_free (buf);
}

GST_END_TEST
static Suite *
dlbutils_suite (void)
{
  Suite *s = suite_create ("dlbutils");
  TCase *tc_general = tcase_create ("general");

  /* add tests to the test case */
  tcase_add_test (tc_general, test_dlb_utils_buffer_data_type);
  tcase_add_test (tc_general, test_dlb_utils_buffer_reordering);

  /* add test case to the suite */
  suite_add_tcase (s, tc_general);

  return s;
}

GST_CHECK_MAIN (dlbutils)
