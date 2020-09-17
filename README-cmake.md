ibus-hangul with CMake
================================================================================

    cd <ibus-hangul-directory>
    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make
    make install

# Test build

    cmake .. -DENABLE_TEST=ON

# Relocate whole installation i.e. rpm or dpkg build

    make install DESTDIR=<...>
