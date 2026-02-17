add_library(mpd MODULE
    gmpd.c
    gmpd.h
    gmpd-private.h
    squeezebox-mpd.c
)

target_include_directories(mpd PRIVATE ${BACKEND_INCLUDE_DIRS} ${GTHREAD_INCLUDE_DIRS})
target_link_libraries(mpd ${BACKEND_LINK_LIBS} ${GTHREAD_LIBRARIES})
target_compile_definitions(mpd PRIVATE HAVE_CONFIG_H BASENAME="mpd")

set_target_properties(mpd PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS mpd LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/mpd)
install(FILES settings-mpd.ui mpd.png DESTINATION ${BACKEND_INSTALL_DIR}/mpd)
