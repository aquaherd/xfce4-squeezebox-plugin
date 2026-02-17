add_library(xfce4-squeezebox-plugin MODULE
    squeezebox.c
    squeezebox.h
    squeezebox-private.h
    settings.c
    notifications.c
    notifications.h
    notifications-binding.h
)

target_include_directories(xfce4-squeezebox-plugin PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${DBUS_GLIB_INCLUDE_DIRS}
    ${GTHREAD_INCLUDE_DIRS}
)

target_link_libraries(xfce4-squeezebox-plugin
    ${COMMON_LINK_LIBS}
    ${LIBXFCE4PANEL_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${DBUS_GLIB_LIBRARIES}
    ${LIBEXO_LIBRARIES}
    ${KEYBINDER_LIBRARIES}
    ${GTHREAD_LIBRARIES}
)

if(LIBGIO_FOUND)
    target_compile_definitions(xfce4-squeezebox-plugin PRIVATE HAVE_GIO=1)
    target_include_directories(xfce4-squeezebox-plugin PRIVATE ${LIBGIO_INCLUDE_DIRS})
    target_link_libraries(xfce4-squeezebox-plugin ${LIBGIO_LIBRARIES})
endif()

if(LIBID3TAG_FOUND)
    target_compile_definitions(xfce4-squeezebox-plugin PRIVATE HAVE_LIBID3TAG=1)
    target_include_directories(xfce4-squeezebox-plugin PRIVATE ${LIBID3TAG_INCLUDE_DIRS})
    target_link_libraries(xfce4-squeezebox-plugin ${LIBID3TAG_LIBRARIES})
endif()

if(LIBZEITGEIST_FOUND)
    target_compile_definitions(xfce4-squeezebox-plugin PRIVATE HAVE_LIBZEITGEIST=1)
    target_include_directories(xfce4-squeezebox-plugin PRIVATE ${LIBZEITGEIST_INCLUDE_DIRS})
    target_link_libraries(xfce4-squeezebox-plugin ${LIBZEITGEIST_LIBRARIES})
endif()

target_compile_definitions(xfce4-squeezebox-plugin PRIVATE
    HAVE_CONFIG_H
)

set_target_properties(xfce4-squeezebox-plugin PROPERTIES
    PREFIX "lib"
    NO_SONAME TRUE
)

install(TARGETS xfce4-squeezebox-plugin
    LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)

# Desktop file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/squeezebox.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/squeezebox.desktop
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squeezebox.desktop
    DESTINATION ${DESKTOP_INSTALL_DIR}
)
