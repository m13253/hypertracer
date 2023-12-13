project('c11threads', ['c'],
  version : '1.0',
  default_options : ['warning_level=3', 'c_std=c17']
)

c11threads_dep = declare_dependency(
  include_directories : include_directories('.'),
  dependencies : dependency('threads'),
)
