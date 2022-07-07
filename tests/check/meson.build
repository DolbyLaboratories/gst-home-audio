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

tests = {
  'libs/meta/dlbaudiometa.c': {},
  'libs/utils/dlbutils.c': {},
  'elements/dlbac3dec.c': {'validate' : 'dlbac3dec'},
  'elements/dlboar.c': {'validate' : 'dlboar'},
  'elements/dlbdap.c': {'validate' : 'dlbdap'},
  'elements/dlbflexr.c': {'validate' : 'dlbflexr'},
}

env = environment()

env.set('GST_PLUGIN_PATH', join_paths(gst_plugins_dlb_build_dir, 'plugins'))
env.append('GST_PLUGIN_PATH', '/usr/local/lib/gstreamer-1.0')

env.set('GSETTINGS_BACKEND', 'memory')
env.set('CK_DEFAULT_TIMEOUT', '60')
env.set('CK_FORK', 'no')

# Skip flexr test on platforms different that x86_64, the config generation
# tools are only available for this platform. Disable forking on qemu.
if host_machine.cpu_family() != 'x86_64'
  env.set('GST_CHECKS_IGNORE', 'test_dlbflexr*')
endif

if get_option('b_sanitize') == 'address'
  libasan = cc.find_library('asan', required: false)
  if libasan.found()
    env.set('G_SLICE', 'always-malloc')
    env.set('G_DEBUG', 'gc-friendly')
    env.set('ASAN_OPTIONS', 'fast_unwind_on_malloc=0:verify_asan_link_order=0')
    env.set('LSAN_OPTIONS', 'suppressions=@0@'.format(join_paths(meson.current_source_dir(), 'lsan.supp')))
    add_test_setup('asan', env: env)
  endif
endif

valgrind = find_program('valgrind', required : false)
if valgrind.found()
  env.set('CK_TIMEOUT_MULTIPLIER', '10')

  add_test_setup('valgrind',
    env : env,
    timeout_multiplier: 10,
    exclude_suites: ['gst-home-audio:python'],
    exe_wrapper : [valgrind,
      '--tool=memcheck',
      '--track-origins=yes',
      '--leak-check=full',
      '--trace-children=yes',
      '--show-possibly-lost=no',
      '--leak-resolution=high',
      '--num-callers=20',
      '--error-exitcode=1',
      '--suppressions=@0@'.format(join_paths(meson.current_source_dir(), 'gst.supp')),
      '--suppressions=@0@'.format(join_paths(meson.current_source_dir(), 'glib.supp')),
      ])
endif

validator = executable('validator', 'validator.c', dependencies: gst_dep)

foreach test, info: tests
    test_name = test.split('.')[0].underscorify()
    element_name = info.get('validate', [])

    exe = executable(test_name, test,
      include_directories : configinc,
                   c_args : ['-DHAVE_CONFIG_H=1'] + test_defines,
             dependencies : test_deps,
    )

    test(test_name, validator, args: [exe.full_path(), element_name], timeout: 60, env: env, suite : 'gstcheck')
endforeach
