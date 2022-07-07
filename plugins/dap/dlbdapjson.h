/*******************************************************************************

 * Dolby Home Audio GStreamer Plugins
 * Copyright (C) 2020-2022, Dolby Laboratories

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

#ifndef _DLBDAPJSON_H_
#define _DLBDAPJSON_H_

#include <glib-object.h>
#include "dlb_dap.h"

#define DLB_DAP_GLOBAL_SETTINGS_TYPE_BOXED      \
  (dlb_dap_global_settings_get_type ())
#define DLB_DAP_VIRTUALIZER_SETTINGS_TYPE_BOXED \
  (dlb_dap_virtualizer_settings_get_type ())
#define DLB_DAP_PROFILE_SETTINGS_TYPE_BOXED     \
  (dlb_dap_profile_settings_get_type ())
#define DLB_DAP_GAIN_SETTINGS_TYPE_BOXED        \
  (dlb_dap_gain_settings_get_type())

typedef struct dlb_dap_global_settings_s
{
  gboolean use_serialized_settings;
  gboolean virtualizer_enable;
  gboolean override_virtualizer_settings;
  /* call g_free () for profile to free */
  gchar  *profile;
} dlb_dap_global_settings;

GType    dlb_dap_global_settings_get_type      (void);

GType    dlb_dap_virtualizer_settings_get_type (void);

GType    dlb_dap_gain_settings_get_type        (void);

GType    dlb_dap_profile_settings_get_type     (void);

gboolean dlb_dap_json_parse_config             (const gchar * filename,
                                                dlb_dap_global_settings *global,
                                                dlb_dap_virtualizer_settings *virtualizer,
                                                dlb_dap_gain_settings *gains,
                                                dlb_dap_profile_settings *profile,
                                                GError **error);

guchar * dlb_dap_json_parse_serialized_config  (const gchar * filename,
                                                gint sample_rate,
                                                gboolean virtualizer_enable,
                                                GError **error);

#endif /* _DAP_DLBDAPJSON_H_ */
