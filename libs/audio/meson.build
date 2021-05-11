dlb_audio_sources = [
  'dlbaudiodecoder.c',
]

dlb_audio_incdir = include_directories('.')

dlb_audio_lib = library('gstdlbaudio', dlb_audio_sources,
  include_directories : [dlb_audio_incdir, configinc],
               c_args : gst_plugins_dlb_args,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep],
              install : true,
)

dlb_audio_dep = declare_dependency(
            link_with : dlb_audio_lib,
  include_directories : dlb_audio_incdir,
         dependencies : dlb_buffer_dep
)
