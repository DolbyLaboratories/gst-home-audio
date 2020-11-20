plugins = ['ac3', 'oar', 'audiodecbin', 'typefind', 'dap']

foreach plugin : plugins
  if not get_option(plugin).disabled()
    subdir(plugin)
  endif
endforeach
