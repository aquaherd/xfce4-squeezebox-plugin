add_library(radiotray MODULE
    squeezebox-radiotray.c
)

target_include_directories(radiotray PRIVATE ${BACKEND_INCLUDE_DIRS})
target_link_libraries(radiotray ${BACKEND_LINK_LIBS})
target_compile_definitions(radiotray PRIVATE HAVE_CONFIG_H BASENAME="radiotray")

set_target_properties(radiotray PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS radiotray LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/radiotray)
install(FILES radiotray.png DESTINATION ${BACKEND_INSTALL_DIR}/radiotray)
