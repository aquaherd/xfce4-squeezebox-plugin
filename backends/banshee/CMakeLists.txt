add_library(banshee MODULE
    squeezebox-ba.c
    banshee-player-binding.h
)

target_include_directories(banshee PRIVATE ${BACKEND_INCLUDE_DIRS})
target_link_libraries(banshee ${BACKEND_LINK_LIBS})
target_compile_definitions(banshee PRIVATE HAVE_CONFIG_H BASENAME="banshee")

set_target_properties(banshee PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS banshee LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/banshee)
install(FILES banshee.png DESTINATION ${BACKEND_INSTALL_DIR}/banshee)
