project('hypertracer', ['c', 'cpp'],
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c17', 'cpp_std=c++20']
)

c11threads_dep = dependency('', required : false)
if not meson.get_compiler('c').has_header('threads.h')
  c11threads_dep = subproject('c11threads').get_variable('c11threads_dep')
endif
deps = [
  c11threads_dep,
  subproject('siphash').get_variable('siphash_dep'),
]

sources = [
  'src/array.c',
  'src/csvread_error.c',
  'src/csvreader.c',
  'src/csvwrite_error.c',
  'src/csvwriter.c',
  'src/hashmap.c',
  'src/logfile.c',
  'src/mpsc.c',
  'src/strbuilder.c',
  'src/string.c',
  'src/tracer.c',
]
install_headers(
  'include/hypertracer',
  'include/hypertracer.h',
  subdir : 'hypertracer'
)

lib = both_libraries('hypertracer', sources,
  install : true,
  dependencies : deps,
  c_shared_args : ['-DBUILDING_HYPERTRACER'],
  include_directories : include_directories('include'),
  gnu_symbol_visibility : 'hidden',
)
hypertracer_dep = declare_dependency(
  include_directories : include_directories('include'),
  link_with : lib
)

lib_cpp = both_libraries('hypertracer_c++', 'src/c++/hypertracer.cpp',
  install : true,
  cpp_shared_args : ['-DBUILDING_HYPERTRACER_CPLUSPLUS'],
  include_directories : include_directories('include'),
  gnu_symbol_visibility : 'hidden',
  link_with : lib.get_static_lib()
)
hypertracer_cpp_dep = declare_dependency(
  include_directories : include_directories('include'),
  link_with : lib_cpp
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
pkg_mod.generate(
  name : 'hypertracer_c++',
  filebase : 'hypertracer_c++',
  description : 'A tracing library for HPC kernel tuning & performance',
  subdirs : 'hypertracer',
  libraries : lib_cpp,
  version : '0.1',
)

subdir('examples')
