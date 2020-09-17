set(project "ibus-hangul")

add_definitions (
    -DHAVE_CONFIG_H
    -DPKGDATADIR=\"${PKGDATADIR}\"
    -DLOCALEDIR=\"${CMAKE_INSTALL_FULL_LOCALEDIR}\"
    -DLIBEXECDIR=\"${CMAKE_INSTALL_FULL_LIBEXECDIR}\"
    -DIBUSHANGUL_DATADIR=\"${CMAKE_INSTALL_FULL_DATADIR}/ibus-hangul\"
)

include_directories(${CMAKE_SOURCE_DIR}/src
    ${HANGUL_INCLUDE_DIRS}
    ${IBUS_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
)

list(APPEND ${project}_SOURCES
    ibus-hangul.c
)

add_executable(${project} ${${project}_SOURCES})

target_link_libraries(${project}
    PRIVATE
        internal
    PUBLIC
        ${IBUS_LIBRARIES}
        ${HANGUL_LIBRARIES}
        ${GTK_LIBRARIES}
)
