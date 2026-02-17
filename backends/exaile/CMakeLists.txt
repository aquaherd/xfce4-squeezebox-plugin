add_library(exaile MODULE
    squeezebox-ex.c
    exaile-player-binding.h
)

target_include_directories(exaile PRIVATE ${BACKEND_INCLUDE_DIRS})
target_link_libraries(exaile ${BACKEND_LINK_LIBS})
target_compile_definitions(exaile PRIVATE HAVE_CONFIG_H BASENAME="exaile")

set_target_properties(exaile PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS exaile LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/exaile)
install(FILES exaile.png DESTINATION ${BACKEND_INSTALL_DIR}/exaile)
