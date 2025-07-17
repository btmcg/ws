project('ws', 'cpp',
  version : '0.1.0',
  default_options : ['warning_level=3',
                     'cpp_std=c++23'])

# Import filesystem module
fs = import('fs')

# Include directories
inc_dir = include_directories('src')

# Find all C++ source files in src directory
src_files = files(
  'src/base64_codec.cpp'
)

# Main executable source files
main_files = src_files + files('src/main.cpp')

# TCP Echo Server source files
tcp_echo_server_files = files(
  'src/tcp_echo_server/tcp_echo_server.cpp',
  'src/tcp_echo_server/main.cpp',
)


# Find spdlog dependency
add_global_arguments('-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE', language : 'cpp')
spdlog_dep = dependency('spdlog', required: true)


# Find Catch2 dependency
catch2_dep = dependency('catch2-with-main', required: false)
if not catch2_dep.found()
  catch2_dep = dependency('catch2', required: false)
endif

# If Catch2 is not found via pkg-config, try to find it as a subproject
if not catch2_dep.found()
  catch2_proj = subproject('catch2', required: false)
  if catch2_proj.found()
    catch2_dep = catch2_proj.get_variable('catch2_with_main_dep')
  endif
endif

# Define the main executable
executable('ws',
  sources : main_files,
  include_directories : inc_dir,
  dependencies : [spdlog_dep],
  install : true)

# Define the TCP echo server executable
executable('tcp_echo_server',
  sources : tcp_echo_server_files,
  include_directories : inc_dir,
  dependencies : [spdlog_dep],
  install : true)

# Optional: Create a library for easier testing
ws_lib = static_library('ws_lib',
  sources : src_files,
  include_directories : inc_dir,
  dependencies : [spdlog_dep])

# Tests configuration
if catch2_dep.found()
  # Test files for each class
  test_files = [
    'tests/test_str_utils.cpp', 
    'tests/test_base64.cpp',
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
  message('Catch2 not found, tests will not be built')
  message('To install Catch2:')
  message('  - Ubuntu/Debian: sudo apt install catch2')
  message('  - Fedora: sudo dnf install catch2-devel')
  message('  - macOS: brew install catch2')
  message('  - Or use Meson wrap: meson wrap install catch2')
endif

# Add option to enable/disable tests
if get_option('enable_tests')
  if not catch2_dep.found()
    error('Tests enabled but Catch2 not found')
  endif
endif
