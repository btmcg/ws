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
  #'src/price_level.cpp',
)

# Main executable source files
main_files = src_files + files('src/main.cpp')

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
  install : true)

# Optional: Create a library for easier testing
itch_parser_lib = static_library('ws_lib',
  sources : src_files,
  include_directories : inc_dir)

# Tests configuration
if catch2_dep.found()
  # Test files for each class
  test_files = [
    # 'tests/test_price_level.cpp', 
    # 'tests/test_order_book.cpp',
    # 'tests/test_messages.cpp'
  ]

  # Create test executables for each test file
  foreach test_file : test_files
    test_name = fs.stem(test_file)

    test_exe = executable(test_name,
      sources : [test_file],
      include_directories : inc_dir,
      link_with : itch_parser_lib,
      dependencies : catch2_dep,
      build_by_default : false)

    test(test_name, test_exe)
  endforeach

  # Optional: Create a single test executable with all tests
  all_test_exe = executable('all_tests',
    sources : test_files,
    include_directories : inc_dir, 
    link_with : itch_parser_lib,
    dependencies : catch2_dep,
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
