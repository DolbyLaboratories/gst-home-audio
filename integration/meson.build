if not get_option('dap').disabled()
  install_data('frontend-dap.py', install_dir : get_option('bindir'), install_mode: 'rw-r--r--')
  install_data('gst-ha-dap', install_dir : get_option('bindir'), install_mode: 'rwxr-xr-x')
  install_subdir('gst_ha_dap', install_dir : get_option('bindir'), install_mode: 'rw-r--r--')
endif

if not get_option('flexr').disabled()
  install_data('frontend-flexr.py', install_dir : get_option('bindir'), install_mode: 'rw-r--r--')
  install_data('gst-ha-flexr', install_dir : get_option('bindir'), install_mode: 'rwxr-xr-x')
  install_subdir('gst_ha_flexr', install_dir : get_option('bindir'), install_mode: 'rw-r--r--')
endif
