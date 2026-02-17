add_library(pragha MODULE
    squeezebox-pr.c
)

target_include_directories(pragha PRIVATE ${BACKEND_INCLUDE_DIRS})
target_link_libraries(pragha ${BACKEND_LINK_LIBS})
target_compile_definitions(pragha PRIVATE HAVE_CONFIG_H BASENAME="pragha")

set_target_properties(pragha PROPERTIES
    PREFIX ""
    NO_SONAME TRUE
)

install(TARGETS pragha LIBRARY DESTINATION ${BACKEND_INSTALL_DIR}/pragha)
install(FILES pragha.png DESTINATION ${BACKEND_INSTALL_DIR}/pragha)
