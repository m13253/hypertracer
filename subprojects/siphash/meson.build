project('siphash', ['c'],
  version : '1.0',
  default_options : ['warning_level=3', 'c_std=c99']
)

sources = [
  'halfsiphash.c',
  'siphash.c',
]
install_headers(
  'halfsiphash.h',
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

test_exe = executable('siphash_test', 
  ['test.c', 'testmain.c'],
  dependencies : siphash_dep,
)
test('siphash_test', test_exe)

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'siphash',
  filebase : 'siphash',
  description : 'High-speed secure pseudorandom function for short messages',
  libraries : lib,
  version : '1.0',
)
