add_library(audacious MODULE
    squeezebox-au.c
    audacious-player-binding.h
)

target_include_directories(audacious PRIVATE ${BACKEND_INCLUDE_DIRS})
target_link_libraries(audacious ${BACKEND_LINK_LIBS})
target_compile_definitions(audacious PRIVATE HAVE_CONFIG_H BASENAME="audacious")

set_target_properties(audacious PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS audacious LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/audacious)
install(FILES audacious.png DESTINATION ${BACKEND_INSTALL_DIR}/audacious)
