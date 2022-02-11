dlb_oar_sources = [
  'dlboar.c',
]

dlb_oar_deps = [
  dlb_oar_dep,
  dlb_meta_dep,
  dlb_utils_dep
]

dlboar = shared_library('gstdlboar', dlb_oar_sources,
               c_args : gst_plugins_dlb_args,
            link_args : gst_plugins_link_args,
  include_directories : configinc,
         dependencies : glib_deps + [gst_base_dep, gst_audio_dep, dlb_oar_deps],
              install : true,
          install_dir : plugins_install_dir
)

plugins += [dlboar]
