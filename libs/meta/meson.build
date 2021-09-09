dlb_meta_sources = [
  'dlbaudiometa.c',
]

dlb_meta_incdir = include_directories('.')

dlb_meta_lib = shared_library('gstdlbmeta', dlb_meta_sources,
  include_directories : [dlb_meta_incdir, configinc],
               c_args : gst_plugins_dlb_args,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep],
              install : true,
)

dlb_meta_dep = declare_dependency(
            link_with : dlb_meta_lib,
  include_directories : [dlb_meta_incdir],
)
