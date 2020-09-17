# vim:set noet ts=4:
# ibus-hangul - The Hangul engine for IBus

cmake_minimum_required(VERSION 3.10)

set(IBUS_HANGUL_VERSION 1.5.4)
project(ibus-hangul VERSION ${IBUS_HANGUL_VERSION})

include(CheckIncludeFiles)
include(CheckIncludeFile)
include(GNUInstallDirs)

option(ENABLE_TEST "Enable unit and regression tests" OFF)

#######################################################################
# Find packages
#######################################################################

find_package(PkgConfig REQUIRED)
find_package(Gettext REQUIRED)

# check ibus
pkg_check_modules(IBUS REQUIRED ibus-1.0>=1.5.4)

# check libhangul
pkg_check_modules(HANGUL REQUIRED libhangul>=0.1.0)

# check gtk
pkg_check_modules(GTK REQUIRED gtk+-3.0>=3.0.0)

# To check for an include file you do this:
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/stah.h" HAVE_SYS_STAT_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

set(ENABLE_NLS 1)
set(GETTEXT_PACKAGE \"ibus-hangul\")
set(PKGDATADIR ${CMAKE_INSTALL_FULL_DATADIR}/ibus-hangul)
set(LIBEXECDIR ${CMAKE_INSTALL_FULL_LIBEXECDIR})
set(DATADIR    ${CMAKE_INSTALL_FULL_DATADIR})

# rpm spec
set(PACKAGE ibus-hangul)
set(VERSION ${IBUS_HANGUL_VERSION})
set(PACKAGE_NAME ibus-hangul)
set(PACKAGE_VERSION ${IBUS_HANGUL_VERSION})
set(PACKAGE_VERSION_MAJOR 1)
set(PACKAGE_VERSION_MINOR 5)

message(STATUS "PKGDATADIOR=${PKGDATADIR}")
message(STATUS "LIBEXECDIR=${CMAKE_INSTALL_FULL_LIBEXECDIR}")
message(STATUS "LOCALEDIR=${CMAKE_INSTALL_FULL_LOCALEDIR}")
message(STATUS "DATADIR=${CMAKE_INSTALL_FULL_DATADIR}")
message(STATUS "IBUSHANGUL_DATADIR=${CMAKE_INSTALL_FULL_DATADIR}/ibus-hangul")

add_subdirectory(src)
add_subdirectory(setup)
add_subdirectory(data)
if (ENABLE_TEST)
    enable_testing()
    add_subdirectory(tests)
endif ()

configure_file(config.h.cmake config.h)
configure_file(ibus-hangul.spec.in ibus-hangul.spec @ONLY)

##### Installation targets #####

install(PROGRAMS ${CMAKE_BINARY_DIR}/src/ibus-engine-hangul DESTINATION ${LIBEXECDIR})
install(FILES ${CMAKE_BINARY_DIR}/src/hangul.xml DESTINATION ${DATADIR}/ibus/component)

# data
install(FILES data/org.freedesktop.ibus.engine.hangul.gschema.xml
    DESTINATION ${DATADIR}/glib-2.0/schemas/)
install(FILES ${CMAKE_BINARY_DIR}/data/org.freedesktop.ibus.engine.hangul.metainfo.xml
    DESTINATION ${DATADIR}/metainfo/)
install(FILES data/symbol.txt DESTINATION ${DATADIR}/ibus-hangul/data/)

# install icon
install(FILES icons/ibus-hangul.png icons/ibus-hangul.svg DESTINATION ${PKGDATADIR}/icons/)
install(FILES icons/ibus-hangul.png DESTINATION ${DATADIR}/icons/hicolor/64x64/apps/)
install(FILES icons/ibus-hangul.svg DESTINATION ${DATADIR}/icons/hicolor/scalable/apps/)

# setup
install(PROGRAMS ${CMAKE_BINARY_DIR}/setup/ibus-setup-hangul DESTINATION ${LIBEXECDIR})
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(CODE "execute_process(COMMAND bash -c \"
    ln -sf ../${CMAKE_INSTALL_LIBEXECDIR}/ibus-setup-hangul \
    $DESTDIR${CMAKE_INSTALL_FULL_BINDIR}/ibus-setup-hangul \")")
install(FILES ${CMAKE_BINARY_DIR}/setup/ibus-setup-hangul.desktop DESTINATION ${DATADIR}/applications/)
install(FILES setup/ibus-setup-hangul.png setup/ibus-setup-hangul.svg DESTINATION ${PKGDATADIR}/icons/)
install(FILES setup/ibus-setup-hangul.png DESTINATION ${DATADIR}/icons/hicolor/64x64/apps/)
install(FILES setup/ibus-setup-hangul.svg DESTINATION ${DATADIR}/icons/hicolor/scalable/apps/)
install(FILES setup/main.py setup/keycapturedialog.py setup/setup.ui
    ${CMAKE_BINARY_DIR}/setup/config.py
    DESTINATION ${DATADIR}/ibus-hangul/setup/)
