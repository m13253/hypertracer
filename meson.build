project('hypertracer', ['cpp'],
  version : '0.1',
  default_options : [
    'b_lto=true',
    'buildtype=debugoptimized',
    'c_std=c17',
    'cpp_std=c++20',
    'optimization=3',
    'warning_level=3',
  ]
)

sources = [
  'src/hypertracer.cpp',
]
install_headers(
  'include/hypertracer',
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
  libraries : lib,
  version : '0.1',
)

subdir('examples')
