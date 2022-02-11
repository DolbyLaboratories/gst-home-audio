# Add distro script helper
if not meson.is_subproject()
  subproj_action = get_option('distsubproj')
  meson.add_dist_script('dist_script.sh', meson.project_source_root(), subproj_action)
endif
