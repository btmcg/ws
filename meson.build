project('ws', 'cpp',
  version : '0.1.0',
  default_options : [
    'b_lto=true',
    'cpp_std=c++23',
    'default_library=static',
    'optimization=3',
    'prefer_static=true',
    'warning_level=3',
    'werror=true',
])

# import filesystem module
fs = import('fs')

# include directories
inc_dir = include_directories('src')

src_util_files = files(
  'src/util/base64_codec.cpp',
  'src/util/sha1.cpp',
)

src_ws_files = files(
  'src/ws/frame.cpp',
  'src/ws/frame_generator.cpp',
)

src_echo_server_files = files(
  'src/echo_server/echo_server.cpp',
  'src/echo_server/main.cpp',
)

# main executable source files
src_main_files = files('src/main.cpp')

cpp_args = [
  # this allows the usage of SPDLOG_TRACE
  '-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE',
  # this tells spdlog to use std::format rather than an external fmt lib
  '-DSPDLOG_USE_STD_FORMAT',
]

addl_warning_flags = [
  '-fno-operator-names',
  '-Wcast-align',
  '-Wcast-qual',
  '-Wdisabled-optimization',
  '-Wempty-body',
  '-Wfloat-equal',
  '-Wformat=2',
  '-Wmissing-include-dirs',
  '-Wno-deprecated-declarations',
  '-Wnon-virtual-dtor',
  '-Woverloaded-virtual',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
  '-Wuninitialized',
  '-Wzero-as-null-pointer-constant',
]
cpp_args += addl_warning_flags

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'gcc'
  cpp_args += [
    '-Wsuggest-final-methods',
    '-Wsuggest-final-types',
    '-Wsuggest-override',
    '-Wuseless-cast',
  ]
endif

add_project_arguments(cpp_args, language : 'cpp')


#
# spdlog
#
spdlog_proj = subproject('spdlog', default_options: [
    'bench=false',
    'default_library=static',
    'example=false', 
    'install=false',
    'tests=disabled',
])
spdlog_dep = spdlog_proj.get_variable('spdlog_dep')


#
# catch2
#
catch2_proj = subproject('catch2', default_options: [
    'tests=false',
    'default_library=static',
])
catch2_dep = catch2_proj.get_variable('catch2_with_main_dep')


# define the main executable
executable('ws-test',
  sources : src_main_files,
  include_directories : inc_dir,
  dependencies : [spdlog_dep],
  install : true)

ws_lib = static_library('ws',
  sources : src_ws_files,
  include_directories : inc_dir,
  dependencies : [spdlog_dep])

util_lib = static_library('util',
  sources : src_util_files,
  include_directories : inc_dir,
  dependencies : [spdlog_dep])

# define the echo server executable
executable('echo_server',
  sources : src_echo_server_files,
  include_directories : inc_dir,
  link_with : [util_lib, ws_lib],
  dependencies : [spdlog_dep],
  install : true)

# Tests configuration
if catch2_dep.found()
  # Test files for each class
  test_files = [
    'tests/util/test_base64_codec.cpp',
    'tests/util/test_byte_buffer.cpp',
    'tests/util/test_sha1.cpp',
    'tests/util/test_str_utils.cpp', 
    'tests/ws/test_frame.cpp',
    'tests/ws/test_frame_generator.cpp',
  ]

  # Create test executables for each test file
  foreach test_file : test_files
    test_name = fs.stem(test_file)

    test_exe = executable(test_name,
      sources : [test_file],
      include_directories : inc_dir,
      link_with : [util_lib, ws_lib],
      dependencies : [catch2_dep, spdlog_dep],
      build_by_default : false)

    test(test_name, test_exe)
  endforeach

  # Optional: Create a single test executable with all tests
  all_test_exe = executable('all_tests',
    sources : test_files,
    include_directories : inc_dir, 
    link_with : [util_lib, ws_lib],
    dependencies : [catch2_dep, spdlog_dep],
    build_by_default : false)

  test('all_tests', all_test_exe)

else
  warning('Catch2 not found, tests will not be built')
endif

# add option to enable/disable tests
if get_option('enable_tests')
  if not catch2_dep.found()
    error('Tests enabled but Catch2 not found')
  endif
endif
