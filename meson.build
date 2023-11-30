project('hypetrace', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c17', 'cpp_std=c++20']
)

sources = [
  'src/array.c',
  'src/csv_reader.c',
  'src/csv_writer.c',
  'src/error.c',
  'src/hashmap.c',
  'src/hypetrace.c',
  'src/siphash.c',
  'src/strbuf.c',
  'src/string.c',
]

lib = static_library('hypetrace', sources,
  install : true,
  include_directories : 'include',
  gnu_symbol_visibility : 'hidden',
)
install_headers('include/hypetrace.h',
  subdir : 'hypetrace'
)

hypetrace_dep = declare_dependency(
  include_directories : include_directories('include'),
  link_with : lib
)

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'hypetrace',
  filebase : 'hypetrace',
  description : 'A tracing & instrumenting library for HPC kernel tuning',
  subdirs : 'hypetrace',
  libraries : lib,
  version : '0.1',
)

subdir('test')
