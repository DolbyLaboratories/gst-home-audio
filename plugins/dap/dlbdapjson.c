/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2020-2021, Dolby Laboratories

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

#include "dlbdapjson.h"

#include <glib/gi18n.h>
#include <json-glib/json-glib.h>


#define DLB_DAP_GLOBAL_SETTINGS_STRING      "global"
#define DLB_DAP_SERIALIZED_SETTINGS_STRING  "serialized-settings"
#define DLB_DAP_VIRTUALIZER_SETTINGS_STRING "virtualizer-settings"
#define DLB_DAP_PROFILE_SETTINGS_STRING     "profile-settings"

static gpointer
dlb_dap_global_settings_copy (gpointer data)
{
  dlb_dap_global_settings *dest = g_slice_dup (dlb_dap_global_settings, data);
  dlb_dap_global_settings *src = (dlb_dap_global_settings *) data;

  dest->profile = g_strdup (src->profile);

  return (gpointer)dest;
}

static void
dlb_dap_global_settings_free (gpointer boxed)
{
  if (G_LIKELY (boxed != NULL)) {
    g_free (((dlb_dap_global_settings *) boxed)->profile);
    g_slice_free (dlb_dap_global_settings, boxed);
  }
}

static gpointer
dlb_dap_virt_settings_copy (gpointer src)
{
  return g_slice_dup (dlb_dap_virtualizer_settings, src);
}

static void
dlb_dap_gains_free (gpointer boxed)
{
  if (G_LIKELY (boxed != NULL))
    g_slice_free (dlb_dap_gain_settings, boxed);
}

static gpointer
dlb_dap_gains_copy (gpointer src)
{
  return g_slice_dup (dlb_dap_gain_settings, src);
}

static void
dlb_dap_virt_settings_free (gpointer boxed)
{
  if (G_LIKELY (boxed != NULL))
    g_slice_free (dlb_dap_virtualizer_settings, boxed);
}

static gpointer
dlb_dap_profile_copy (gpointer src)
{
  return g_slice_dup (dlb_dap_profile_settings, src);
}

static void
dlb_dap_profile_free (gpointer boxed)
{
  if (G_LIKELY (boxed != NULL))
    g_slice_free (dlb_dap_profile_settings, boxed);
}

#define MAKE_FILL_JSON_ARRAY_HELPERS(type)                                     \
static JsonArray *                                                             \
fill_json_array_##type (const type *tab, guint size)                           \
{                                                                              \
  JsonArray *array = json_array_new ();                                        \
  for (guint i = 0; i < size; ++i)                                             \
    json_array_add_int_element (array, tab[i]);                                \
  return array;                                                                \
}

typedef unsigned int uint;

MAKE_FILL_JSON_ARRAY_HELPERS (int)
MAKE_FILL_JSON_ARRAY_HELPERS (uint)
#define MAKE_FILL_ARRAY_FROM_JSON_HELPERS(type)                                \
static void                                                                    \
fill_array_from_json_##type (type * array, gsize size, JsonArray *json_array)  \
{                                                                              \
  for (guint i = 0; i < size; ++i)                                             \
    array[i] = json_array_get_int_element (json_array, i);                     \
}
MAKE_FILL_ARRAY_FROM_JSON_HELPERS (int)
MAKE_FILL_ARRAY_FROM_JSON_HELPERS (uint)

static JsonNode *dlb_dap_global_settings_serialize (gconstpointer boxed)
{
  const dlb_dap_global_settings *global = boxed;
  JsonObject *object;
  JsonNode *node;

  if (boxed == NULL)
    return json_node_new (JSON_NODE_NULL);

  object = json_object_new ();
  node = json_node_new (JSON_NODE_OBJECT);

  json_object_set_boolean_member (object, "use-serialized-settings",
      global->use_serialized_settings);
  json_object_set_boolean_member (object, "virtualizer-enable",
      global->virtualizer_enable);
  json_object_set_boolean_member (object, "override-virtualizer-settings",
      global->override_virtualizer_settings);
  json_object_set_string_member (object, "profile", global->profile);

  json_node_take_object (node, object);
  return node;
}

static gpointer
dlb_dap_global_settings_deserialize (JsonNode * node)
{
  JsonObject *object;
  dlb_dap_global_settings *global;

  if (json_node_get_node_type (node) != JSON_NODE_OBJECT)
    return NULL;

  object = json_node_get_object (node);
  global = g_slice_new (dlb_dap_global_settings);

  global->use_serialized_settings =
      json_object_get_boolean_member (object, "use-serialized-settings");
  global->virtualizer_enable =
      json_object_get_boolean_member (object, "virtualizer-enable");
  global->override_virtualizer_settings =
      json_object_get_boolean_member (object, "override-virtualizer-settings");
  global->profile = g_strdup (
      json_object_get_string_member (object, "profile"));

  return global;
}

/* Virtualizer setting deserialize */
static JsonNode *
dlb_dap_virt_settings_serialize (gconstpointer boxed)
{
  const dlb_dap_virtualizer_settings *virtcfg = boxed;
  JsonObject *object;
  JsonNode *node;

  if (boxed == NULL)
    return json_node_new (JSON_NODE_NULL);

  object = json_object_new ();
  node = json_node_new (JSON_NODE_OBJECT);

  json_object_set_int_member (object, "front-speaker-angle",
      virtcfg->front_speaker_angle);
  json_object_set_int_member (object, "surround-speaker-angle",
      virtcfg->surround_speaker_angle);
  json_object_set_int_member (object, "rear-surround-speaker-angle",
      virtcfg->rear_surround_speaker_angle);
  json_object_set_int_member (object, "height-speaker-angle",
      virtcfg->height_speaker_angle);
  json_object_set_int_member (object, "rear-height-speaker-angle",
      virtcfg->rear_height_speaker_angle);
  json_object_set_int_member (object, "height-filter-enable",
      virtcfg->height_filter_enable);

  json_node_take_object (node, object);
  return node;
}

static gpointer
dlb_dap_virt_settings_deserialize (JsonNode * node)
{
  JsonObject *object;
  dlb_dap_virtualizer_settings *virtcfg;

  if (json_node_get_node_type (node) != JSON_NODE_OBJECT)
    return NULL;

  object = json_node_get_object (node);
  virtcfg = g_slice_new (dlb_dap_virtualizer_settings);

  virtcfg->front_speaker_angle =
      json_object_get_int_member (object, "front-speaker-angle");
  virtcfg->surround_speaker_angle =
      json_object_get_int_member (object, "surround-speaker-angle");
  virtcfg->rear_surround_speaker_angle =
      json_object_get_int_member (object, "rear-surround-speaker-angle");
  virtcfg->height_speaker_angle =
      json_object_get_int_member (object, "height-speaker-angle");
  virtcfg->rear_height_speaker_angle =
      json_object_get_int_member (object, "rear-height-speaker-angle");
  virtcfg->height_filter_enable =
      json_object_get_int_member (object, "height-filter-enable");

  return virtcfg;
}

static JsonNode *
dlb_dap_gain_settings_serialize (gconstpointer boxed)
{
  const dlb_dap_gain_settings *gains = boxed;
  JsonObject *object;
  JsonNode *node;

  if (boxed == NULL)
    return json_node_new (JSON_NODE_NULL);

  object = json_object_new ();
  node = json_node_new (JSON_NODE_OBJECT);

  json_object_set_int_member (object, "postgain", gains->postgain);
  json_object_set_int_member (object, "pregain", gains->pregain);
  json_object_set_int_member (object, "system-gain", gains->system_gain);

  json_node_take_object (node, object);
  return node;
}

static gpointer
dlb_dap_gain_settings_deserialize (JsonNode * node)
{
  JsonObject *object;
  dlb_dap_gain_settings *gains;

  if (json_node_get_node_type (node) != JSON_NODE_OBJECT)
    return NULL;

  object = json_node_get_object (node);
  gains = g_slice_new (dlb_dap_gain_settings);

  gains->postgain = json_object_get_int_member (object, "postgain");
  gains->pregain = json_object_get_int_member (object, "pregain");
  gains->system_gain = json_object_get_int_member (object, "system-gain");

  return gains;
}

static JsonNode *
dlb_dap_profile_serialize (gconstpointer boxed)
{
  const dlb_dap_profile_settings *profile = boxed;
  JsonObject *object;
  JsonNode *node;
  JsonArray *array;

  if (boxed == NULL)
    return json_node_new (JSON_NODE_NULL);

  object = json_object_new ();
  node = json_node_new (JSON_NODE_OBJECT);

  json_object_set_int_member (object, "bass-enhancer-enable",
      profile->bass_enhancer_enable);
  json_object_set_int_member (object, "bass-enhancer-boost",
      profile->bass_enhancer_boost);
  json_object_set_int_member (object, "bass-enhancer-cutoff-frequency",
      profile->bass_enhancer_cutoff_frequency);
  json_object_set_int_member (object, "bass-enhancer-width",
      profile->bass_enhancer_width);
  json_object_set_int_member (object, "calibration-boost",
      profile->calibration_boost);
  json_object_set_int_member (object, "dialog-enhancer-enable",
      profile->dialog_enhancer_enable);
  json_object_set_int_member (object, "dialog-enhancer-amount",
      profile->dialog_enhancer_amount);
  json_object_set_int_member (object, "dialog-enhancer-ducking",
      profile->dialog_enhancer_ducking);
  json_object_set_int_member (object, "graphic-equalizer-enable",
      profile->graphic_equalizer_enable);
  array =
      fill_json_array_uint (profile->graphic_equalizer_bands,
      profile->graphic_equalizer_bands_num);
  json_object_set_array_member (object, "graphic-equalizer-bands", array);
  array =
      fill_json_array_int (profile->graphic_equalizer_gains,
      profile->graphic_equalizer_bands_num);
  json_object_set_array_member (object, "graphic-equalizer-gains", array);
  json_object_set_int_member (object, "ieq-enable", profile->ieq_enable);
  json_object_set_int_member (object, "ieq-amount", profile->ieq_amount);
  array = fill_json_array_uint (profile->ieq_bands, profile->ieq_bands_num);
  json_object_set_array_member (object, "ieq-bands", array);
  array = fill_json_array_int (profile->ieq_gains, profile->ieq_bands_num);
  json_object_set_array_member (object, "ieq-bands", array);
  json_object_set_int_member (object, "mi-dialog-enhancer-steering-enable",
      profile->mi_dialog_enhancer_steering_enable);
  json_object_set_int_member (object, "mi-dv-leveler-steering-enable",
      profile->mi_dv_leveler_steering_enable);
  json_object_set_int_member (object, "mi-ieq-steering-enable",
      profile->mi_ieq_steering_enable);
  json_object_set_int_member (object, "mi-surround-compressor-steering-enable",
      profile->mi_surround_compressor_steering_enable);
  json_object_set_int_member (object, "surround-boost",
      profile->surround_boost);
  json_object_set_int_member (object,
      "surround-decoder-center-spreading-enable",
      profile->surround_decoder_center_spreading_enable);
  json_object_set_int_member (object, "surround-decoder-enable",
      profile->surround_decoder_enable);
  json_object_set_int_member (object, "volmax-boost", profile->volmax_boost);
  json_object_set_int_member (object, "volume-leveler-enable",
      profile->volume_leveler_enable);
  json_object_set_int_member (object, "volume-leveler-amount",
      profile->volume_leveler_amount);

  json_node_take_object (node, object);
  return node;
}

static gpointer
dlb_dap_profile_deserialize (JsonNode * node)
{
  JsonObject *object;
  JsonArray *array;
  dlb_dap_profile_settings *profile;

  if (json_node_get_node_type (node) != JSON_NODE_OBJECT)
    return NULL;

  object = json_node_get_object (node);
  profile = g_slice_new (dlb_dap_profile_settings);
  dlb_dap_profile_settings_init (profile);

  profile->bass_enhancer_enable =
      json_object_get_int_member (object, "bass-enhancer-enable");
  profile->bass_enhancer_boost =
      json_object_get_int_member (object, "bass-enhancer-boost");
  profile->bass_enhancer_cutoff_frequency =
      json_object_get_int_member (object, "bass-enhancer-cutoff-frequency");
  profile->bass_enhancer_width =
      json_object_get_int_member (object, "bass-enhancer-width");
  profile->calibration_boost =
      json_object_get_int_member (object, "calibration-boost");
  profile->dialog_enhancer_enable =
      json_object_get_int_member (object, "dialog-enhancer-enable");
  profile->dialog_enhancer_amount =
      json_object_get_int_member (object, "dialog-enhancer-amount");
  profile->dialog_enhancer_ducking =
      json_object_get_int_member (object, "dialog-enhancer-ducking");
  profile->graphic_equalizer_enable =
      json_object_get_int_member (object, "graphic-equalizer-enable");
  array = json_object_get_array_member (object, "graphic-equalizer-bands");
  profile->graphic_equalizer_bands_num = json_array_get_length (array);
  fill_array_from_json_uint (profile->graphic_equalizer_bands,
      profile->graphic_equalizer_bands_num, array);
  array = json_object_get_array_member (object, "graphic-equalizer-gains");
  fill_array_from_json_int (profile->graphic_equalizer_gains,
      profile->graphic_equalizer_bands_num, array);
  profile->ieq_enable = json_object_get_int_member (object, "ieq-enable");
  profile->ieq_amount = json_object_get_int_member (object, "ieq-amount");
  array = json_object_get_array_member (object, "ieq-bands");
  profile->ieq_bands_num = json_array_get_length (array);
  fill_array_from_json_uint (profile->ieq_bands, profile->ieq_bands_num, array);
  array = json_object_get_array_member (object, "ieq-gains");
  fill_array_from_json_int (profile->ieq_gains, profile->ieq_bands_num, array);
  profile->mi_dialog_enhancer_steering_enable =
      json_object_get_int_member (object, "mi-dialog-enhancer-steering-enable");
  profile->mi_dv_leveler_steering_enable =
      json_object_get_int_member (object, "mi-dv-leveler-steering-enable");
  profile->mi_ieq_steering_enable =
      json_object_get_int_member (object, "mi-ieq-steering-enable");
  profile->mi_surround_compressor_steering_enable =
      json_object_get_int_member (object,
      "mi-surround-compressor-steering-enable");
  profile->surround_boost =
      json_object_get_int_member (object, "surround-boost");
  profile->surround_decoder_center_spreading_enable =
      json_object_get_int_member (object,
      "surround-decoder-center-spreading-enable");
  profile->surround_decoder_enable =
      json_object_get_int_member (object, "surround-decoder-enable");
  profile->volmax_boost = json_object_get_int_member (object, "volmax-boost");
  profile->volume_leveler_enable =
      json_object_get_int_member (object, "volume-leveler-enable");
  profile->volume_leveler_amount =
      json_object_get_int_member (object, "volume-leveler-amount");

  return profile;
}

GType
dlb_dap_global_settings_get_type (void)
{
  static GType b_type = 0;

  if (G_UNLIKELY (b_type == 0)) {
    b_type = g_boxed_type_register_static ("DlbDapGlobalSettings",
        dlb_dap_global_settings_copy, dlb_dap_global_settings_free);

    json_boxed_register_serialize_func (b_type, JSON_NODE_OBJECT,
        dlb_dap_global_settings_serialize);
    json_boxed_register_deserialize_func (b_type, JSON_NODE_OBJECT,
        dlb_dap_global_settings_deserialize);
  }

  return b_type;
}

GType
dlb_dap_virtualizer_settings_get_type (void)
{
  static GType b_type = 0;

  if (G_UNLIKELY (b_type == 0)) {
    b_type = g_boxed_type_register_static ("DlbDapVirtualizerSettings",
        dlb_dap_virt_settings_copy, dlb_dap_virt_settings_free);

    json_boxed_register_serialize_func (b_type, JSON_NODE_OBJECT,
        dlb_dap_virt_settings_serialize);
    json_boxed_register_deserialize_func (b_type, JSON_NODE_OBJECT,
        dlb_dap_virt_settings_deserialize);
  }

  return b_type;
}

GType
dlb_dap_gain_settings_get_type (void)
{
  static GType b_type = 0;

  if (G_UNLIKELY (b_type == 0)) {
    b_type = g_boxed_type_register_static ("DlbDapGainSettings",
        dlb_dap_gains_copy, dlb_dap_gains_free);

    json_boxed_register_serialize_func (b_type, JSON_NODE_OBJECT,
        dlb_dap_gain_settings_serialize);
    json_boxed_register_deserialize_func (b_type, JSON_NODE_OBJECT,
        dlb_dap_gain_settings_deserialize);
  }

  return b_type;
}

GType
dlb_dap_profile_settings_get_type (void)
{
  static GType b_type = 0;

  if (G_UNLIKELY (b_type == 0)) {
    b_type = g_boxed_type_register_static ("DlbDapProfileSettings",
        dlb_dap_profile_copy, dlb_dap_profile_free);

    json_boxed_register_serialize_func (b_type, JSON_NODE_OBJECT,
        dlb_dap_profile_serialize);
    json_boxed_register_deserialize_func (b_type, JSON_NODE_OBJECT,
        dlb_dap_profile_deserialize);
  }

  return b_type;
}

gboolean
dlb_dap_json_parse_config (const gchar * filename,
    dlb_dap_global_settings * global, dlb_dap_virtualizer_settings * virt,
    dlb_dap_gain_settings *gains, dlb_dap_profile_settings * profile,
    GError ** error)
{
  JsonParser *parser;
  JsonNode *root;
  JsonObject *jsonobj;

  parser = json_parser_new ();
  if (!json_parser_load_from_file (parser, filename, error)) {
    g_object_unref (parser);
    return FALSE;
  }

  root = json_parser_get_root (parser);
  jsonobj = json_node_get_object (root);

  if (global && json_object_has_member (jsonobj, "global")) {
    dlb_dap_global_settings *g;

    root = json_object_get_member (jsonobj, "global");
    g = json_boxed_deserialize (DLB_DAP_GLOBAL_SETTINGS_TYPE_BOXED, root);

    global->override_virtualizer_settings = g->override_virtualizer_settings;
    global->use_serialized_settings = g->use_serialized_settings;
    global->virtualizer_enable = g->virtualizer_enable;
    global->profile = g_strdup (g->profile);
    g_boxed_free (DLB_DAP_GLOBAL_SETTINGS_TYPE_BOXED, g);
  }

  if (virt && json_object_has_member (jsonobj, "virtualizer-settings")) {
    dlb_dap_virtualizer_settings *v;

    root = json_object_get_member (jsonobj, "virtualizer-settings");
    v = json_boxed_deserialize (DLB_DAP_VIRTUALIZER_SETTINGS_TYPE_BOXED, root);

    *virt = *v;
    g_boxed_free (DLB_DAP_VIRTUALIZER_SETTINGS_TYPE_BOXED, v);
  }

  if (gains && json_object_has_member (jsonobj, "gain-settings")) {
    dlb_dap_gain_settings *g;

    root = json_object_get_member (jsonobj, "gain-settings");
    g = json_boxed_deserialize (DLB_DAP_GAIN_SETTINGS_TYPE_BOXED, root);

    *gains = *g;
    g_boxed_free (DLB_DAP_GAIN_SETTINGS_TYPE_BOXED, g);
  }

  if (profile && json_object_has_member (jsonobj, "profiles")) {
    root = json_object_get_member (jsonobj, "profiles");
    jsonobj = json_node_get_object (root);
    if (json_object_has_member (jsonobj, global->profile)) {
      dlb_dap_profile_settings *p;

      root = json_object_get_member (jsonobj, global->profile);
      p = json_boxed_deserialize (DLB_DAP_PROFILE_SETTINGS_TYPE_BOXED, root);

      *profile = *p;
      g_boxed_free (DLB_DAP_PROFILE_SETTINGS_TYPE_BOXED, p);
    }
  }

  g_object_unref (parser);
  return TRUE;
}

guchar *
dlb_dap_json_parse_serialized_config (const gchar * filename,
    gint sample_rate, gboolean virtualizer_enable, GError ** error)
{
  JsonParser *parser;
  JsonNode *root;
  JsonObject *jsonobj;

  guchar *data = NULL;
  gsize data_size;

  parser = json_parser_new ();
  if (!json_parser_load_from_file (parser, filename, error)) {
    g_object_unref (parser);
    return FALSE;
  }

  root = json_parser_get_root (parser);
  jsonobj = json_node_get_object (root);

  if (json_object_has_member (jsonobj, "serialized-settings")) {
    gchar *sr = g_strdup_printf ("sr-%d", sample_rate);

    root = json_object_get_member (jsonobj, "serialized-settings");
    jsonobj = json_node_get_object (root);

    if (json_object_has_member (jsonobj, sr)) {
      const gchar *base64;
      const gchar *config = virtualizer_enable ? "virt-enable" : "virt-disable";

      root = json_object_get_member (jsonobj, sr);
      jsonobj = json_node_get_object (root);

      base64 = json_object_get_string_member (jsonobj, config);
      data = g_base64_decode (base64, &data_size);
    }

    g_free (sr);
  }

  g_object_unref (parser);
  return data;
}
