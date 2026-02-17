add_library(quodlibet MODULE
    squeezebox-ql.c
)

target_include_directories(quodlibet PRIVATE ${BACKEND_INCLUDE_DIRS})
target_link_libraries(quodlibet ${BACKEND_LINK_LIBS})
target_compile_definitions(quodlibet PRIVATE HAVE_CONFIG_H BASENAME="quodlibet")

set_target_properties(quodlibet PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS quodlibet LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/quodlibet)
install(FILES quodlibet.png DESTINATION ${BACKEND_INSTALL_DIR}/quodlibet)
