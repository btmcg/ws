# ws
web sockets

# just run the damn thing
`meson setup build`
`meson compile -C build`
`build/echo_server`

# build optimized version
`meson setup build_gcc --native-file=gcc.ini`
`meson compile -C build_gcc`
or
`meson setup build_clang --native-file=clang.ini`
`meson compile -C build_clang`

# build debug version
`meson setup build_gcc --native-file=gcc.ini -Doptimization=0`
`meson compile -C build_gcc`
or
`meson setup build_clang --native-file=clang.ini -Doptimization=0`
`meson compile -C build_clang`

# run tests
`meson test -C <build_dir>`
