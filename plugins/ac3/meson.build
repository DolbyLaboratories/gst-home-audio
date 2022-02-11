dlbac3dec_sources = [
  'dlbac3dec.c',
  'dlbac3dec.h',
]

ac3dec_deps = [
  dlb_udc_dep,
  dlb_audio_dep,
  dlb_meta_dep,
  dlb_utils_dep,
]

dlbac3dec = library('gstdlbac3dec', dlbac3dec_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep, ac3dec_deps],
              install : true,
          install_dir : plugins_install_dir,
)


dlbac3parse_sources = [
  'dlbac3parse.c',
  'dlbac3parse.h',
]

ac3parse_deps = [
  dlb_audio_parser_dep,
]

dlbac3parse = library('gstdlbac3parse', dlbac3parse_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep, ac3parse_deps, 
                        gst_pbutils_dep],
              install : true,
          install_dir : plugins_install_dir,
)

plugins += [dlbac3dec, dlbac3parse]
