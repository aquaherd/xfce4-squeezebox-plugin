add_library(muine MODULE
    squeezebox-mu.c
    muine-player-binding.h
)

target_include_directories(muine PRIVATE ${BACKEND_INCLUDE_DIRS})
target_link_libraries(muine ${BACKEND_LINK_LIBS})
target_compile_definitions(muine PRIVATE HAVE_CONFIG_H BASENAME="muine")

set_target_properties(muine PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS muine LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/muine)
install(FILES muine.png DESTINATION ${BACKEND_INSTALL_DIR}/muine)
