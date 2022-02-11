dlbaudiodecbin_sources = [
  'dlbaudiodecbin.h',
  'dlbaudiodecbin.c'
]

dlbaudiodecbin_deps = [
  dlb_audio_dep,
  dlb_utils_dep,
  dlb_meta_dep,
]

dlbaudiodecbin = shared_library('gstdlbaudiodecbin', dlbaudiodecbin_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep, gst_pbutils_dep, dlbaudiodecbin_deps],
              install : true,
          install_dir : plugins_install_dir,
)

plugins += [dlbaudiodecbin]
