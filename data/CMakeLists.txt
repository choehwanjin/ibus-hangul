set(project data)

# schema
set(_schema org.freedesktop.ibus.engine.hangul.gschema.xml)

# appstream
set(_metainfo org.freedesktop.ibus.engine.hangul.metainfo.xml)
set(_metainfo_in ${CMAKE_CURRENT_SOURCE_DIR}/${_metainfo}.in)

add_custom_target(${_metainfo} ALL
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --xml --template ${_metainfo_in} -d ${CMAKE_SOURCE_DIR}/po -o ${_metainfo}
    DEPENDS ${_metainfo}
    VERBATIM
)
