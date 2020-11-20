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
#include <gst/audio/audio.h>

#include "dlbaudiometa.h"

static const gsize BUFFER_SIZE = 1024;
static const gsize BPF = 4;

static GstBuffer *buffer;

static const gsize PAYLOAD_SIZE = 128;
static gpointer payload_data;

static void
dlb_audio_meta_test_setup (void)
{
  buffer = gst_buffer_new_allocate (NULL, BUFFER_SIZE, NULL);
  payload_data = g_malloc0 (PAYLOAD_SIZE);
  memset (payload_data, 0xab, PAYLOAD_SIZE);
}

static void
dlb_audio_meta_test_teardown (void)
{
  gst_buffer_unref (buffer);
  g_free (payload_data);
}

GST_START_TEST (test_dlbaudiometa_add_get_one)
{
  DlbObjectAudioMeta *meta;

  /* check if inital buffer does not contain meta */
  fail_unless_equals_int (0, gst_buffer_get_n_meta (buffer,
          DLB_OBJECT_AUDIO_META_API_TYPE));

  /* add and create a single meta */
  meta =
      dlb_audio_object_meta_add (buffer, payload_data, PAYLOAD_SIZE, 100, BPF);
  fail_if (NULL == meta);
  fail_unless_equals_int (PAYLOAD_SIZE, meta->size);
  fail_unless_equals_int (100, meta->offset);
  fail_unless_equals_int (BPF, meta->bpf);

  /* check if buffer contains single meta */
  fail_unless_equals_int (1, dlb_audio_object_meta_get_n (buffer));

  /* validate signle meta in the buffer */
  meta = dlb_audio_object_meta_get (buffer);
  fail_if (NULL == meta);
  fail_unless_equals_int (PAYLOAD_SIZE, meta->size);
  fail_unless_equals_int (100, meta->offset);
  fail_unless_equals_int (BPF, meta->bpf);
}

#define NUM_METADATA (100)
GST_END_TEST
GST_START_TEST (test_dlbaudiometa_add_get_many)
{
  DlbObjectAudioMeta *meta;
  GList *list = NULL;
  gpointer iter = NULL;
  guint i, n;

  n = dlb_audio_object_meta_get_n (buffer);

  /* check if inital buffer does not contain meta */
  fail_unless_equals_int (0, n);

  /* add multiple metadatas */
  for (i = 0; i < NUM_METADATA; ++i) {
    meta =
        dlb_audio_object_meta_add (buffer, payload_data, PAYLOAD_SIZE, i, BPF);

    fail_if (NULL == meta);
    fail_unless_equals_int (PAYLOAD_SIZE, meta->size);
    fail_unless_equals_int (i, meta->offset);
  }

  /* check if buffer contains n meta */
  n = dlb_audio_object_meta_get_n (buffer);
  fail_unless_equals_int (NUM_METADATA, n);

  /* sort meta based on seqnum */
  while ((meta = dlb_audio_object_meta_iterate (buffer, &iter))) {
    list = g_list_prepend (list, meta);
  }

  list = g_list_sort (list, (GCompareFunc) gst_meta_compare_seqnum);

  for (i = 0; i < n; ++i) {
    meta = (DlbObjectAudioMeta *) g_list_nth_data (list, i);

    fail_if (NULL == meta);
    fail_unless_equals_int (PAYLOAD_SIZE, meta->size);
    fail_unless_equals_int (BPF, meta->bpf);
    fail_unless_equals_int (i, meta->offset);
  }

  g_list_free (list);
}

GST_END_TEST
GST_START_TEST (test_dlbaudiometa_copy)
{
  DlbObjectAudioMeta *meta;
  GstBuffer *copy;

  /* check if inital buffer does not contain meta */
  fail_unless_equals_int (0, gst_buffer_get_n_meta (buffer,
          DLB_OBJECT_AUDIO_META_API_TYPE));

  /* add and create a single meta */
  meta =
      dlb_audio_object_meta_add (buffer, payload_data, PAYLOAD_SIZE, 100, BPF);

  copy = gst_buffer_copy (buffer);
  fail_if (NULL == copy);

  /* check if buffer contains single meta */
  fail_unless_equals_int (1, dlb_audio_object_meta_get_n (copy));

  /* validate signle meta in the buffer */
  meta = dlb_audio_object_meta_get (copy);
  fail_if (NULL == meta);
  fail_unless_equals_int (PAYLOAD_SIZE, meta->size);
  fail_unless_equals_int (100, meta->offset);
  fail_unless_equals_int (BPF, meta->bpf);

  gst_buffer_unref (copy);
}

GST_END_TEST
GST_START_TEST (test_dlbaudiometa_copy_region)
{
  DlbObjectAudioMeta *meta;
  GstBuffer *copy;

  /* check if inital buffer does not contain meta */
  fail_unless_equals_int (0, gst_buffer_get_n_meta (buffer,
          DLB_OBJECT_AUDIO_META_API_TYPE));

  /* add and create a single meta */
  meta =
      dlb_audio_object_meta_add (buffer, payload_data, PAYLOAD_SIZE, 100, BPF);
  fail_if (NULL == meta);

  /* copy region before metadata offset */
  copy =
      gst_buffer_copy_region (buffer, GST_BUFFER_COPY_META, 0, 100 * BPF - 1);
  fail_if (NULL == copy);
  fail_unless_equals_int (0, dlb_audio_object_meta_get_n (copy));
  gst_buffer_unref (copy);

  /* copy region with metadata */
  copy = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_META, 0, 100 * BPF);
  fail_if (NULL == copy);
  fail_unless_equals_int (1, dlb_audio_object_meta_get_n (copy));
  meta = dlb_audio_object_meta_get (copy);
  fail_if (NULL == meta);
  fail_unless_equals_int (100, meta->offset);
  gst_buffer_unref (copy);

  /* copy region with offset with metadata */
  copy =
      gst_buffer_copy_region (buffer, GST_BUFFER_COPY_META, 1, 100 * BPF - 1);
  fail_if (NULL == copy);
  fail_unless_equals_int (1, dlb_audio_object_meta_get_n (copy));
  meta = dlb_audio_object_meta_get (copy);
  fail_if (NULL == meta);
  fail_unless_equals_int (99, meta->offset);
  gst_buffer_unref (copy);

  /* copy byte with metadata */
  copy = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_META, 100 * BPF, 1);
  fail_if (NULL == copy);
  fail_unless_equals_int (1, dlb_audio_object_meta_get_n (copy));
  meta = dlb_audio_object_meta_get (copy);
  fail_if (NULL == meta);
  fail_unless_equals_int (0, meta->offset);
  gst_buffer_unref (copy);

  /* copy region after metadata */
  copy =
      gst_buffer_copy_region (buffer, GST_BUFFER_COPY_META, 100 * BPF + 1, 100);
  fail_if (NULL == copy);
  fail_unless_equals_int (0, dlb_audio_object_meta_get_n (copy));
  gst_buffer_unref (copy);
}
GST_END_TEST

static Suite *
dlbaudiometa_suite (void)
{
  Suite *s = suite_create ("dlbaudiometa");
  TCase *tc_general = tcase_create ("general");

  /* set setup and teardown functions for each test in the test case */
  tcase_add_checked_fixture (tc_general, dlb_audio_meta_test_setup,
      dlb_audio_meta_test_teardown);

  /* add tests to the test case */
  tcase_add_test (tc_general, test_dlbaudiometa_add_get_one);
  tcase_add_test (tc_general, test_dlbaudiometa_add_get_many);
  tcase_add_test (tc_general, test_dlbaudiometa_copy);
  tcase_add_test (tc_general, test_dlbaudiometa_copy_region);

  /* add test case to the suite */
  suite_add_tcase (s, tc_general);

  return s;
}

GST_CHECK_MAIN (dlbaudiometa)
