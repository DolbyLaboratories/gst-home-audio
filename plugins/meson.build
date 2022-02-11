plugin_opts = ['ac3', 'oar', 'flexr', 'audiodecbin', 'typefind', 'dap']

foreach plugin : plugin_opts
  if not get_option(plugin).disabled()
    subdir(plugin)
  endif
endforeach
