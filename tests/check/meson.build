test_defines = [
    '-DGST_TEST_FILES_PATH="' + meson.current_source_dir() + '/../files"',
    '-DGST_FGEN_FILES_PATH="' + meson.current_build_dir() + '/../../subprojects/dlb_flexr/tests"',
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
  'elements/dlbflexr.c',
]

env = environment()

env.set('GST_PLUGIN_PATH', join_paths(gst_plugins_dlb_build_dir, 'plugins'))
env.set('GSETTINGS_BACKEND', 'memory')
env.set('CK_DEFAULT_TIMEOUT', '60')
env.set('CK_FORK', 'no')

# Skip flexr test on platforms different that x86_64, the config generation
# tools are only available for this platform. Disable forking on qemu.
if host_machine.cpu_family() != 'x86_64'
  env.set('GST_CHECKS_IGNORE', 'test_dlb_flexr*')
endif

foreach test : tests
  test_name = test.split('.')[0].underscorify()

  exe = executable(test_name, test,
    include_directories : configinc,
                 c_args : ['-DHAVE_CONFIG_H=1'] + test_defines,
           dependencies : test_deps,
  )

  test(test_name, exe, timeout: 60, env: env)
endforeach
