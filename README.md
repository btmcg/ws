# web sockets

# just run the damn thing
`meson setup build`
`meson compile -C build`
`build/echo_server`

# build optimized version
`meson setup build_gcc --native-file=gcc_release.ini`
`meson compile -C build_gcc`
or
`meson setup build_clang --native-file=clang_release.ini`
`meson compile -C build_clang`

# build debug version
`meson setup build_gcc --native-file=gcc_debug.ini`
`meson compile -C build_gcc`
or
`meson setup build_clang --native-file=clang_debug.ini`
`meson compile -C build_clang`

# run tests
`meson test -C <build_dir>`
