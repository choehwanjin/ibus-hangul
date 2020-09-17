set(project setup)

find_program(PYTHON python)

set(SETUP_GETTEXT_PACKAGE ${GETTEXT_PACKAGE})
set(SETUP_LOCALEDIR ${CMAKE_INSTALL_FULL_LOCALEDIR})
set(SETUP_PKGDATADIR ${PKGDATADIR})

configure_file(config.py.in config.py)
configure_file(ibus-setup-hangul.in ibus-setup-hangul)

# ibus-setup-hangul.desktop is always built
set(_desktop ibus-setup-hangul.desktop)
set(_desktop_in ${CMAKE_CURRENT_SOURCE_DIR}/ibus-setup-hangul.desktop.in)

add_custom_target(${_desktop} ALL
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --desktop --template ${_desktop_in} -d ${CMAKE_SOURCE_DIR}/po -o ${_desktop}
    DEPENDS ${_desktop}
    VERBATIM
)
