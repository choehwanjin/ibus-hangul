# vim:set noet ts=4:

############################################################
# Create a library
############################################################

project(internal)

add_library(internal STATIC
	engine.c
	engine.h
	ustring.c
	ustring.h
	i18n.h
)

add_definitions (
        -DHAVE_CONFIG_H
        -DPKGDATADIR=\"${PKGDATADIR}\"
        -DLOCALEDIR=\"${CMAKE_INSTALL_FULL_LOCALEDIR}\"
        -DLIBEXECDIR=\"${CMAKE_INSTALL_FULL_LIBEXECDIR}\"
        -DIBUSHANGUL_DATADIR=\"${CMAKE_INSTALL_FULL_DATADIR}/ibus-hangul\"
)

target_include_directories(internal
    PUBLIC
        ${CMAKE_BINARY_DIR}
        ${HANGUL_INCLUDE_DIRS}
        ${IBUS_INCLUDE_DIRS}
)

############################################################
# Create an executable
############################################################

project(ibus-engine-hangul)

add_executable(ibus-engine-hangul
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)

target_link_libraries(ibus-engine-hangul
    PRIVATE
        internal
    PUBLIC
        ${IBUS_LIBRARIES}
        ${HANGUL_LIBRARIES}
)

set(libexecdir ${LIBEXECDIR})
configure_file(hangul.xml.in.in hangul.xml.in)
unset(libexecdir)

add_custom_target(hangul.xml ALL
    COMMAND sed -e "s/\\\\\"/\"/g" ${CMAKE_CURRENT_BINARY_DIR}/hangul.xml.in > hangul.xml
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/hangul.xml.in
    COMMENT "Generating hangul.xml"
    VERBATIM
)
