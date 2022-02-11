dlb_flexr_sources = [
  'dlbflexr.c',
]

dlb_flexr_deps = [
  dlb_flexr_dep,
  dlb_meta_dep,
  dlb_utils_dep
]

dlbflexr = shared_library('gstdlbflexr', dlb_flexr_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep, dlb_flexr_deps],
              install : true,
          install_dir : plugins_install_dir
)

plugins += [dlbflexr]
