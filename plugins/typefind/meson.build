dlb_typefind_sources = [
  'dlbtypefindfunctions.c',
]

dlb_typefind_deps = [
  dlb_audio_parser_dep,
]

dlbtypefind = shared_library('gstdlbtypefind', dlb_typefind_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep, dlb_typefind_deps],
              install : true,
          install_dir : plugins_install_dir
)

plugins += [dlbtypefind]
