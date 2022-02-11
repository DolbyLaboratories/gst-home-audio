dlb_utils_sources = [
  'dlbutils.c',
]

dlb_utils_deps = [
  dlb_buffer_dep,
]

dlb_utils_incdir = include_directories('.')

dlb_utils_lib = library('gstdlbutils', dlb_utils_sources,
  include_directories : [dlb_utils_incdir, configinc],
               c_args : gst_plugins_dlb_args,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep, dlb_utils_deps],
              install : true,
)

dlb_utils_dep = declare_dependency(
            link_with : dlb_utils_lib,
  include_directories : dlb_utils_incdir,
         dependencies : dlb_buffer_dep
)
