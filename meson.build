project('hypertracer', ['cpp'],
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++20']
)

sources = [
  'src/cow.cpp',
  'src/env.cpp',
  'src/file.cpp',
  'src/hypertracer.cpp',
  'src/mpsc.cpp',
  'src/msg.cpp',
  'src/panic.cpp',
]
install_headers(
  'include/cow',
  'include/env',
  'include/file',
  'include/hypertracer',
  'include/mpsc',
  'include/msg',
  'include/panic',
  subdir : 'hypertracer'
)

lib = static_library('hypertracer', sources,
  install : true,
  include_directories : include_directories('include'),
  gnu_symbol_visibility : 'hidden',
)
hypertracer_dep = declare_dependency(
  include_directories : include_directories('include'),
  link_with : lib
)

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'hypertracer',
  filebase : 'hypertracer',
  description : 'A tracing library for HPC kernel tuning & performance',
  subdirs : 'hypertracer',
  libraries : lib,
  version : '0.1',
)

subdir('examples')
