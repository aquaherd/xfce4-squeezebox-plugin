add_library(rhythmbox MODULE
    squeezebox-rb.c
    rb-shell-binding.h
    rb-shell-player-binding.h
)

target_include_directories(rhythmbox PRIVATE ${BACKEND_INCLUDE_DIRS})
target_link_libraries(rhythmbox ${BACKEND_LINK_LIBS})
target_compile_definitions(rhythmbox PRIVATE HAVE_CONFIG_H BASENAME="rhythmbox")

set_target_properties(rhythmbox PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS rhythmbox LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/rhythmbox)
install(FILES rhythmbox.png DESTINATION ${BACKEND_INSTALL_DIR}/rhythmbox)
