tests = [
  'sanity.py'
]

foreach test : tests
  test_name = 'integration_' + test.split('.')[0].underscorify()
  env = environment()

  env.set('GST_PLUGIN_PATH', join_paths(gst_plugins_dlb_build_dir, 'plugins'))
  env.set('GST_TEST_FILES_PATH', join_paths(meson.current_source_dir(), '../files'))

  test(test_name, py, args : files(test), env : env, suite: 'python')
endforeach
