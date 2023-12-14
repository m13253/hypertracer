project('siphash', ['c'],
  version : '1.0',
  default_options : ['warning_level=3', 'c_std=c17']
)

sources = [
  'siphash.c'
]
install_headers(
  'siphash.h',
)

lib = static_library('siphash', sources,
  install : true,
  include_directories : include_directories('.'),
  gnu_symbol_visibility : 'hidden',
)
siphash_dep = declare_dependency(
  include_directories : include_directories('.'),
  link_with : lib
)

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'siphash',
  filebase : 'siphash',
  description : 'High-speed secure pseudorandom function for short messages',
  libraries : lib,
  version : '1.0',
)
