# ws
web sockets


# build optimized version with gcc
`meson setup build_gcc`
`meson compile -C build_gcc`
`meson test -C build_gcc`

# build optimized version with clang
`CC=clang CXX=clang++ meson setup build_clang`
`meson compile -C build_clang`
`meson test -C build_clang`

# build debug
`meson setup build_gcc -Doptimization=0`
`meson compile -C build_gcc`
`meson test -C build_gcc`
