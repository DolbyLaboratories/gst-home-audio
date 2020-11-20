test_defines = [
    '-DGST_TEST_FILES_PATH="' + meson.current_source_dir() + '/../files"',
]

test_deps = [
  gst_check_dep,
  gst_audio_dep,
  dlb_meta_dep,
  dlb_utils_dep,
  dlb_audio_dep,
]

tests = [
  'libs/meta/dlbaudiometa.c',
  'libs/utils/dlbutils.c',
  'elements/dlbac3dec.c',
  'elements/dlboar.c',
  'elements/dlbdap.c',
]

foreach test : tests
  test_name = test.split('.')[0].underscorify()

  env = environment()

  env.set('GST_PLUGIN_PATH', join_paths(gst_plugins_dlb_build_dir, 'plugins'))
  env.set('GSETTINGS_BACKEND', 'memory')

  exe = executable(test_name, test,
    include_directories : configinc,
                 c_args : ['-DHAVE_CONFIG_H=1'] + test_defines,
           dependencies : test_deps,
  )

  test(test_name, exe, timeout: 10 * 60, env: env)
endforeach
