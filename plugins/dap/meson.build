dlb_dap_sources = [
  'dlbdap.c',
  'dlbdapjson.c'
]

dlb_dap_deps = [
  dlb_dap_dep,
  dlb_utils_dep
]

dlbdap = shared_library('gstdlbdap', dlb_dap_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep, dlb_dap_deps],
              install : true,
          install_dir : plugins_install_dir
)

plugins += [dlbdap]
