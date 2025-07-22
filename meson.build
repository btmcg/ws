project('ws', 'cpp',
  version : '0.1.0',
  default_options : ['warning_level=3',
                     'cpp_std=c++23'])

# import filesystem module
fs = import('fs')

# include directories
inc_dir = include_directories('src')

# find all C++ source files in src directory
src_files = files(
  'src/base64_codec.cpp',
  'src/sha1.cpp',
)

# main executable source files
main_files = src_files + files('src/main.cpp')

# echo server source files
echo_server_files = files(
  'src/echo_server/echo_server.cpp',
  'src/echo_server/main.cpp',
)

# use spdlog from subproject
add_global_arguments([
  '-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE',
  '-DSPDLOG_USE_STD_FORMAT'
  ], language : 'cpp')
spdlog_proj = subproject('spdlog', default_options: [
    'tests=disabled',
    'example=disabled', 
    'bench=disabled',
    'install=disabled'
])
spdlog_dep = spdlog_proj.get_variable('spdlog_dep')

# use catch2 from subproject
catch2_proj = subproject('catch2', default_options: [
    'tests=disabled'
])
catch2_dep = catch2_proj.get_variable('catch2_with_main_dep')

# define the main executable
executable('ws',
  sources : main_files,
  include_directories : inc_dir,
  dependencies : [spdlog_dep],
  install : true)

ws_lib = static_library('ws_lib',
  sources : src_files,
  include_directories : inc_dir,
  dependencies : [spdlog_dep])

# define the echo server executable
executable('echo_server',
  sources : echo_server_files,
  include_directories : inc_dir,
  link_with : ws_lib,
  dependencies : [spdlog_dep],
  install : true)

# Tests configuration
if catch2_dep.found()
  # Test files for each class
  test_files = [
    'tests/test_str_utils.cpp', 
    'tests/test_base64_codec.cpp',
    'tests/test_sha1.cpp',
  ]

  # Create test executables for each test file
  foreach test_file : test_files
    test_name = fs.stem(test_file)

    test_exe = executable(test_name,
      sources : [test_file],
      include_directories : inc_dir,
      link_with : ws_lib,
      dependencies : [catch2_dep, spdlog_dep],
      build_by_default : false)

    test(test_name, test_exe)
  endforeach

  # Optional: Create a single test executable with all tests
  all_test_exe = executable('all_tests',
    sources : test_files,
    include_directories : inc_dir, 
    link_with : ws_lib,
    dependencies : [catch2_dep, spdlog_dep],
    build_by_default : false)

  test('all_tests', all_test_exe)

else
  warning('Catch2 not found, tests will not be built')
endif

# Add option to enable/disable tests
if get_option('enable_tests')
  if not catch2_dep.found()
    error('Tests enabled but Catch2 not found')
  endif
endif
