project('gst-home-audio', 'c', 'cpp',
         version : '0.8.1',
   meson_version : '>= 0.54.0',
 default_options : ['warning_level=1', 'buildtype=debugoptimized'])

system = host_machine.system()
version = meson.project_version()

if host_machine.system() != 'linux'
  error('only linux is currently supported')
endif

glib_req = '>= 2.40.0'
gst_req = '>= 1.16.0'

cc = meson.get_compiler('c')
host_system = host_machine.system()

py_mod = import('python')
py = py_mod.find_installation()

# config.h
core_conf = configuration_data()
core_conf.set_quoted('VERSION', meson.project_version())
core_conf.set_quoted('PACKAGE', meson.project_name())
core_conf.set_quoted('LICENSE', 'LGPL')
core_conf.set_quoted('ORIGIN', 'http://dolby.com/')

configinc = include_directories('.')

gst_plugins_dlb_args = ['-DHAVE_CONFIG_H']
gst_plugins_link_args = []

if (get_option('buildtype') == 'release')
  gst_plugins_dlb_args += '-DG_DISABLE_ASSERT'
endif

warning_flags = [
  '-Wmissing-declarations',
  '-Wredundant-decls',
  '-Wwrite-strings',
  '-Winit-self',
  '-Wmissing-include-dirs',
  '-Wno-multichar',
  '-Wvla',
  '-Wpointer-arith',
  '-Wmissing-prototypes',
  '-Wdeclaration-after-statement',
  '-Wold-style-definition',
  '-Waggregate-return',
]

foreach extra_arg : warning_flags
  if cc.has_argument (extra_arg)
    add_project_arguments([extra_arg], language: 'c')
  endif
endforeach

# libm, pthread
libm = cc.find_library('m', required : false)
dl_dep = cc.find_library('dl', required : false)
thread_dep = dependency('threads')

if dl_dep.found()
  core_conf.set('HAVE_DLADDR', 1)
endif

# GLib, gobject
glib_deps = [dependency('glib-2.0', version : glib_req, fallback: ['glib', 'libglib_dep']),
             dependency('gobject-2.0', fallback: ['glib', 'libgobject_dep']),
             dependency('json-glib-1.0')]

# GStreamer dependencies
gst_dep = dependency('gstreamer-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_dep'])
gst_base_dep = dependency('gstreamer-base-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_base_dep'])
gst_controller_dep = dependency('gstreamer-controller-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_controller_dep'])
gst_audio_dep = dependency('gstreamer-audio-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_audio_dep'])
gst_video_dep = dependency('gstreamer-video-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_video_dep'])
gst_check_dep = dependency('gstreamer-check-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_check_dep'])
gst_pbutils_dep = dependency('gstreamer-pbutils-1.0', version : gst_req,
  fallback: ['gstreamer', 'gst_pbutils_dep'])

pkgconfig = import('pkgconfig')
plugins_install_dir = '@0@/gstreamer-1.0'.format(get_option('libdir'))
plugins_pkgconfig_install_dir = join_paths(plugins_install_dir, 'pkgconfig')

# set variable that points to this projects build directory for tests
gst_plugins_dlb_build_dir = meson.current_build_dir()

# subprojects
dlb_audio_parser_proj = subproject('dlb_audio_parser', version: '>= 1.0.0', required : false)
dlb_buffer_proj = subproject('dlb_buffer', required : false)
dlb_oar_proj = subproject('dlb_oar', version : '2.5.0', required : false)
dlb_evo_proj = subproject('dlb_evo', version : '1.3', required : false)
dlb_udc_proj = subproject('dlb_udc', version : '4.10.0', required : false)
dlb_dap_proj = subproject('dlb_dap', version : '2.9.0.2', required : false)

proj_map = {
  'dlb_audio_parser': {'proj': dlb_audio_parser_proj, 'dep': []},
  'dlb_buffer': {'proj': dlb_buffer_proj, 'dep': []},
  'dlb_oar': {'proj': dlb_oar_proj, 'dep': []},
  'dlb_evo': {'proj': dlb_evo_proj, 'dep': []},
  'dlb_udc': {'proj': dlb_udc_proj, 'dep': []},
  'dlb_dap': {'proj': dlb_dap_proj, 'dep': []},
}

foreach name, info : proj_map
  proj = info.get('proj')
  dep = info.get('dep')

  if proj.found()
    dep = proj.get_variable(name + '_dep')
  else
    lib = static_library(name + '_shim', 'shim'/name + '.c',
            c_args : ['-DHAVE_CONFIG_H'],
      dependencies : dl_dep
    )
    dep = declare_dependency(
                link_with : lib,
      include_directories : include_directories('shim')
    )
    core_conf.set(name.to_upper() + '_OPEN_DYNLIB', 1)
  endif
  proj_map += {name: proj_map[name] + {'proj': proj, 'dep': dep}}
endforeach

dlb_audio_parser_dep = proj_map.get('dlb_audio_parser').get('dep')
dlb_buffer_dep = proj_map.get('dlb_buffer').get('dep')
dlb_oar_dep = proj_map.get('dlb_oar').get('dep')
dlb_evo_dep = proj_map.get('dlb_evo').get('dep')
dlb_udc_dep = proj_map.get('dlb_udc').get('dep')
dlb_dap_dep = proj_map.get('dlb_dap').get('dep')

subdir('libs')
subdir('plugins')
subdir('tests')
subdir('integration')

# Use core_conf after all subdirs have set values
configure_file(input: 'config.h.in', output : 'config.h', configuration : core_conf)

# Add distro script helper
subproj_action = get_option('distsubproj')
meson.add_dist_script('scripts/dist_script.sh', meson.source_root(), subproj_action)
