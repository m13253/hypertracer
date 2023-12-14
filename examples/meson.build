copycsv = executable('copycsv', 'copycsv.c',
  dependencies : [hypertracer_dep]
)

copycsv_cpp = executable('copycsv_c++', 'copycsv_c++.cpp',
  dependencies : [hypertracer_cpp_dep]
)

trace = executable('trace', 'trace.c',
  dependencies : [hypertracer_dep, dependency('threads')]
)

trace = executable('trace_c++', 'trace_c++.cpp',
  dependencies : [hypertracer_cpp_dep]
)
